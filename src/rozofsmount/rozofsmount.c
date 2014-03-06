/*
  Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
  This file is part of Rozofs.

  Rozofs is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation, version 2.

  Rozofs is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include <inttypes.h>
#include <assert.h>
#include <semaphore.h>
#include <mntent.h>
#include <sys/resource.h>

#include <rozofs/rozofs_debug_ports.h>
#include <rozofs/rozofs_timer_conf.h>
#include <rozofs/core/rozofs_timer_conf_dbg.h>

#include "rozofs_fuse.h"
#include "rozofsmount.h"
#include "rozofs_sharedmem.h"
#include "rozofs_modeblock_cache.h"
#include "rozofs_cache.h"
#include "rozofs_rw_load_balancing.h"
#include "rozofs_reload_export_gateway_conf.h"

#define hash_xor8(n)    (((n) ^ ((n)>>8) ^ ((n)>>16) ^ ((n)>>24)) & 0xff)
#define INODE_HSIZE 8192
#define PATH_HSIZE  8192

// Filesystem source (first field in /etc/mtab)
#define FSNAME "rozofs"

// Filesystem type (third field in /etc/mtab)
// If the kernel suppports it,
///etc/mtab and /proc/mounts will show the filesystem type as fuse.rozofs
#define FSTYPE "rozofs"

// File for list all mounted file systems
#define MOUNTED_FS_FILE_CHECK "/proc/mounts"

#define FUSE28_DEFAULT_OPTIONS "default_permissions,allow_other,fsname="FSNAME",subtype="FSTYPE",big_writes"""
#define FUSE27_DEFAULT_OPTIONS "default_permissions,allow_other,fsname="FSNAME",subtype="FSTYPE""""

#define CACHE_TIMEOUT 10.0

#define ROZOFSMOUNT_MAX_TX 32

#define CONNECTION_THREAD_TIMESPEC  2

#define STORCLI_STARTER  "storcli_starter.sh"
#define STORCLI_KILLER  "storcli_killer.sh"
#define STORCLI_EXEC "storcli"

static SVCXPRT *rozofsmount_profile_svc = 0;
int rozofs_rotation_read_modulo = 0;
static char *mountpoint = NULL;
    
DEFINE_PROFILING(mpp_profiler_t) = {0};

sem_t *semForEver; /**< semaphore used for stopping rozofsmount: typically on umount */


uint64_t   rozofs_client_hash=0;
/**______________________________________________________________________________
*/
/**
*  Compute the client hash

  @param h: hostname string
  @param instance: instance id
  
  @retval hash value
*/
static inline uint64_t rozofs_client_hash_compute(char * hostname, int instance) {
    unsigned char *d = (unsigned char *) hostname;
    uint64_t h;

    h = 2166136261U;
    /*
     ** hash on hostname
     */
    for (; *d != 0; d++) {
        h = (h * 16777619)^ *d;
    }
    /*
     ** hash on instance id
     */
    h = (h * 16777619)^ instance;
    return h;
}     
/*
 *________________________________________________________
 */

/*
 ** API to be called for stopping rozofsmount

 @param none
 @retval none
 */
void rozofs_exit() {
    if (semForEver != NULL) {
        sem_post(semForEver);
        for (;;) {
            severe("rozofsmount exit required.");
            sleep(10);
        }
        fatal("semForEver is not initialized.");
    }
}

extern void rozofsmount_profile_program_1(struct svc_req *rqstp, SVCXPRT *ctl_svc);

static void usage() {
    fprintf(stderr, "ROZOFS options:\n");
    fprintf(stderr, "    -H EXPORT_HOST\t\tdefine address (or dns name) where exportd daemon is running (default: rozofsexport) equivalent to '-o exporthost=EXPORT_HOST'\n");
    fprintf(stderr, "    -E EXPORT_PATH\t\tdefine path of an export see exportd (default: /srv/rozofs/exports/export) equivalent to '-o exportpath=EXPORT_PATH'\n");
    fprintf(stderr, "    -P EXPORT_PASSWD\t\tdefine passwd used for an export see exportd (default: none) equivalent to '-o exportpasswd=EXPORT_PASSWD'\n");
    fprintf(stderr, "    -o rozofsbufsize=N\t\tdefine size of I/O buffer in KiB (default: 256)\n");
    fprintf(stderr, "    -o rozofsminreadsize=N\tdefine minimum read size on disk in KiB (default value is same as the option rozofsbufsize)\n");
    fprintf(stderr, "    -o rozofsmaxwritepending=N\tdefine the number of write request(s) that can be sent for an open file from the rozofsmount toward the storcli asynchronously (default: %u)\n", ROZOFSMOUNT_MAX_TX);
    fprintf(stderr, "    -o rozofsmaxretry=N\t\tdefine number of retries before I/O error is returned (default: 50)\n");
    fprintf(stderr, "    -o rozofsexporttimeout=N\tdefine timeout (s) for exportd requests (default: 25)\n");
    fprintf(stderr, "    -o rozofsstoragetimeout=N\tdefine timeout (s) for IO storaged requests (default: 3)\n");
    fprintf(stderr, "    -o rozofsstorclitimeout=N\tdefine timeout (s) for IO storcli requests (default: 10)\n");
    fprintf(stderr, "    -o rozofsattrtimeout=N\tdefine timeout (s) for which file/directory attributes are cached (default: 10)\n");
    fprintf(stderr, "    -o rozofsentrytimeout=N\tdefine timeout (s) for which name lookups will be cached (default: 10)\n");
    fprintf(stderr, "    -o debug_port=N\t\tdefine the base debug port for rozofsmount (default: none)\n");
    fprintf(stderr, "    -o instance=N\t\tdefine instance number (default: 0)\n");
    fprintf(stderr, "    -o rozofscachemode=N\tdefine the cache mode: 0: no cache, 1: direct_io, 2: keep_cache (default: 0)\n");
    fprintf(stderr, "    -o rozofsmode=N\t\tdefine the operating mode of rozofsmount: 0: filesystem, 1: block mode (default: 0)\n");
    fprintf(stderr, "    -o rozofsnbstorcli=N\tdefine the number of storcli process(es) to use (default: 1)\n");
    fprintf(stderr, "    -o rozofsshaper=N\t\tdefine the storcli shaper configuration (default: 1)\n");
    fprintf(stderr, "    -o rozofsrotate=N\t\tdefine the modulo on read distribution rotation (default: 0)\n");
    fprintf(stderr, "    -o posixlock\t\tactive support for POSIX file lock\n");
    fprintf(stderr, "    -o bsdlock\t\t\tactive support for BSD file lock\n");
}

static rozofsmnt_conf_t conf;

int rozofs_cache_mode  = 0;  /**< 0: no option on open/create, 1: direct_io; 2: keep_cache */
int rozofs_mode  = 0;  /**< 0:filesystem, 1: block mode */

enum {
    KEY_EXPORT_HOST,
    KEY_EXPORT_PATH,
    KEY_EXPORT_PASSWD,
    KEY_HELP,
    KEY_VERSION,
    KEY_DEBUG_PORT,
};

#define MYFS_OPT(t, p, v) { t, offsetof(struct rozofsmnt_conf, p), v }

static struct fuse_opt rozofs_opts[] = {
    MYFS_OPT("exporthost=%s", host, 0),
    MYFS_OPT("exportpath=%s", export, 0),
    MYFS_OPT("exportpasswd=%s", passwd, 0),
    MYFS_OPT("rozofsbufsize=%u", buf_size, 0),
    MYFS_OPT("rozofsminreadsize=%u", min_read_size, 0),
    MYFS_OPT("rozofsmaxwritepending=%u", max_write_pending, 0),
    MYFS_OPT("rozofsnbstorcli=%u", nbstorcli, 0),    
    MYFS_OPT("rozofsmaxretry=%u", max_retry, 0),
    MYFS_OPT("rozofsexporttimeout=%u", export_timeout, 0),
    MYFS_OPT("rozofsstoragetimeout=%u", storage_timeout, 0),
    MYFS_OPT("rozofsstorclitimeout=%u", storcli_timeout, 0),
    MYFS_OPT("rozofsattrtimeout=%u", attr_timeout, 0),
    MYFS_OPT("rozofsentrytimeout=%u", entry_timeout, 0),
    MYFS_OPT("debug_port=%u", dbg_port, 0),
    MYFS_OPT("instance=%u", instance, 0),
    MYFS_OPT("rozofscachemode=%u", cache_mode, 0),
    MYFS_OPT("rozofsmode=%u", fs_mode, 0),
    MYFS_OPT("nbcores=%u", nb_cores, 0),
    MYFS_OPT("rozofsshaper=%u", shaper, 0),
    MYFS_OPT("rozofsrotate=%u", rotate, 0),
    MYFS_OPT("posixlock", posix_file_lock, 1),
    MYFS_OPT("bsdlock", bsd_file_lock, 1),
    MYFS_OPT("grpquota", quota, 2),
    MYFS_OPT("noquota", quota, 0),
    MYFS_OPT("quota", quota, 3),
    MYFS_OPT("usrquota", quota, 1),

    FUSE_OPT_KEY("-H ", KEY_EXPORT_HOST),
    FUSE_OPT_KEY("-E ", KEY_EXPORT_PATH),
    FUSE_OPT_KEY("-P ", KEY_EXPORT_PASSWD),

    FUSE_OPT_KEY("-V", KEY_VERSION),
    FUSE_OPT_KEY("--version", KEY_VERSION),
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_END
};

static int myfs_opt_proc(void *data, const char *arg, int key,
        struct fuse_args *outargs) {
    (void) data;
    switch (key) {
        case FUSE_OPT_KEY_OPT:
            return 1;
        case FUSE_OPT_KEY_NONOPT:
            return 1;
        case KEY_EXPORT_HOST:
            if (conf.host == NULL) {
                conf.host = strdup(arg + 2);
            }
            return 0;
        case KEY_EXPORT_PATH:
            if (conf.export == NULL) {
                conf.export = strdup(arg + 2);
            }
            return 0;
        case KEY_EXPORT_PASSWD:
            if (conf.passwd == NULL) {
                conf.passwd = strdup(arg + 2);
            }
            return 0;
        case KEY_HELP:
            fuse_opt_add_arg(outargs, "-h"); // PRINT FUSE HELP
            fuse_parse_cmdline(outargs, NULL, NULL, NULL);
            fuse_mount(NULL, outargs);
            usage(); // PRINT ROZOFS USAGE
            exit(1);
        case KEY_VERSION:
            fprintf(stderr, "rozofs version %s\n", VERSION);
            fuse_opt_add_arg(outargs, "--version"); // PRINT FUSE VERSION
            fuse_parse_cmdline(outargs, NULL, NULL, NULL);
            exit(0);
    }
    return 1;
}

/**< structure associated to exportd, needed for communication */
exportclt_t exportclt; 

list_t inode_entries;
htable_t htable_inode;
htable_t htable_fid;
uint64_t rozofs_ientries_count = 0;

static void rozofs_ll_init(void *userdata, struct fuse_conn_info *conn) {
    int *piped = (int *) userdata;
    char s;
    (void) conn;
    if (piped[1] >= 0) {
        s = 0;
        if (write(piped[1], &s, 1) != 1) {
            warning("rozofs_ll_init: pipe write error: %s", strerror(errno));
        }
        close(piped[1]);
    }
}

void rozofs_ll_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup) {
    ientry_t *ie;

    START_PROFILING(rozofs_ll_forget);

    DEBUG("forget :%lu, nlookup: %lu", ino, nlookup);
    if ((ie = get_ientry_by_inode(ino))) {
        assert(ie->nlookup >= nlookup);
        DEBUG("forget :%lu, ie lookup: %lu", ino, ie->nlookup);
        if ((ie->nlookup -= nlookup) == 0) {
            DEBUG("del entry for %lu", ino);
            del_ientry(ie);
            if (ie->db.p != NULL) {
                free(ie->db.p);
                ie->db.p = NULL;
            }
            free(ie);
        }
    }

    STOP_PROFILING(rozofs_ll_forget);
    fuse_reply_none(req);
}



/*#warning fake untested function.

void rozofs_ll_fsyncdir(fuse_req_t req, fuse_ino_t ino, int datasync,
        struct fuse_file_info *fi) {
    START_PROFILING(rozofs_ll_fsyncdir);
    fuse_reply_err(req, 0);
    STOP_PROFILING(rozofs_ll_fsyncdir);
}

#warning fake untested function.

void rozofs_ll_ioctl(fuse_req_t req, fuse_ino_t ino, int cmd, void *arg,
        struct fuse_file_info *fi, unsigned flags,
        const void *in_buf, size_t in_bufsz, size_t out_bufsz) {
    START_PROFILING(rozofs_ll_ioctl);
    fuse_reply_ioctl(req, 0, in_buf, out_bufsz);
    STOP_PROFILING(rozofs_ll_ioctl);
}

#warning fake untested function.

void rozofs_ll_access(fuse_req_t req, fuse_ino_t ino, int mask) {
    START_PROFILING(rozofs_ll_access);
    fuse_reply_err(req, 0);
    STOP_PROFILING(rozofs_ll_access);
}*/

static SVCXPRT *rozofsmount_create_rpc_service(int port) {
    int sock;
    int one = 1;
    struct sockaddr_in sin;

    /* Give the socket a name. */
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Create the socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        severe("Can't create socket: %s.", strerror(errno));
        return NULL;
    }

    /* Set socket options */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof (int));
    setsockopt(sock, SOL_TCP, TCP_DEFER_ACCEPT, (char *) &one, sizeof (int));
    setsockopt(sock, SOL_TCP, TCP_NODELAY, (char *) &one, sizeof (int));

    /* Bind the socket */
    if (bind(sock, (struct sockaddr *) &sin, sizeof (struct sockaddr)) < 0) {
        severe("Couldn't bind to tcp port %d", port);
        return NULL;
    }

    /* Creates a TCP/IP-based RPC service transport */
    return svctcp_create(sock, ROZOFS_RPC_BUFFER_SIZE, ROZOFS_RPC_BUFFER_SIZE);

}

void rozofmount_profiling_thread_run(void *args) {

    int *port = args;

    rozofsmount_profile_svc = rozofsmount_create_rpc_service(*port);
    if (!rozofsmount_profile_svc) {
        severe("can't create monitoring service: %s", strerror(errno));
    }

    /* Associates ROZOFSMOUNT_PROFILE_PROGRAM and ROZOFSMOUNT_PROFILE_VERSION
     * with the service dispatch procedure, rozofsmount_profile_program_1.
     * Here protocol is zero, the service is not registered with
     *  the portmap service */
    if (!svc_register(rozofsmount_profile_svc, ROZOFSMOUNT_PROFILE_PROGRAM,
            ROZOFSMOUNT_PROFILE_VERSION,
            rozofsmount_profile_program_1, 0)) {
        severe("can't register service : %s", strerror(errno));
    }

    svc_run();
    DEBUG("REACHED !!!!");
    /* NOT REACHED */
}
#define DISPLAY_UINT32_CONFIG(field)   pChar += sprintf(pChar,"%-25s = %u\n",#field, conf.field); 
#define DISPLAY_STRING_CONFIG(field) \
  if (conf.field == NULL) pChar += sprintf(pChar,"%-25s = NULL\n",#field);\
  else                    pChar += sprintf(pChar,"%-25s = %s\n",#field,conf.field); 
  
void show_start_config(char * argv[], uint32_t tcpRef, void *bufRef) {
  char *pChar = uma_dbg_get_buffer();
  
  DISPLAY_STRING_CONFIG(host);
  DISPLAY_STRING_CONFIG(export);
  DISPLAY_STRING_CONFIG(passwd);  
  DISPLAY_UINT32_CONFIG(buf_size);
  DISPLAY_UINT32_CONFIG(min_read_size);
  DISPLAY_UINT32_CONFIG(max_write_pending);
  DISPLAY_UINT32_CONFIG(nbstorcli);
  DISPLAY_UINT32_CONFIG(max_retry);
  DISPLAY_UINT32_CONFIG(dbg_port);
  DISPLAY_UINT32_CONFIG(instance);  
  DISPLAY_UINT32_CONFIG(export_timeout);
  DISPLAY_UINT32_CONFIG(storcli_timeout);
  DISPLAY_UINT32_CONFIG(storage_timeout);
  DISPLAY_UINT32_CONFIG(fs_mode); 
  DISPLAY_UINT32_CONFIG(cache_mode);  
  DISPLAY_UINT32_CONFIG(attr_timeout);
  DISPLAY_UINT32_CONFIG(entry_timeout);
  DISPLAY_UINT32_CONFIG(nb_cores);
  DISPLAY_UINT32_CONFIG(shaper);  
  DISPLAY_UINT32_CONFIG(rotate);  
  DISPLAY_UINT32_CONFIG(posix_file_lock);  
  DISPLAY_UINT32_CONFIG(bsd_file_lock);  
  uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}  
/*__________________________________________________________________________
*/  
static char * show_rotate_modulo_help(char * pChar) {
  pChar += sprintf(pChar,"usage:\n");
  pChar += sprintf(pChar,"rotateModulo set <value> : set new rotate modulo value\n");
  pChar += sprintf(pChar,"rotateModulo             : display rotate modulo value\n");  
  return pChar; 
}
void show_rotate_modulo(char * argv[], uint32_t tcpRef, void *bufRef) {
  char *pChar = uma_dbg_get_buffer();
  int   new_val;
   
   if (argv[1] !=NULL)
   {
    if (strcmp(argv[1],"set")==0) {
      errno = 0;       
      new_val = (int) strtol(argv[2], (char **) NULL, 10);   
      if (errno != 0) {
        pChar += sprintf(pChar, "bad value %s\n",argv[2]);
	pChar = show_rotate_modulo_help(pChar);
        uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
	return;     
      } 
      rozofs_rotation_read_modulo = new_val;        
      uma_dbg_send(tcpRef, bufRef, TRUE, "New rotate modulo set to %d\n",rozofs_rotation_read_modulo);    
      return;     
   }
    /*
    ** Help
    */
    pChar = show_rotate_modulo_help(pChar);
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
    return;   
  }
  uma_dbg_send(tcpRef, bufRef, TRUE, "rotation modulo is %d\n",rozofs_rotation_read_modulo);    
  return;     
}   
/*__________________________________________________________________________
*/
void reset_lock_stat(void);
char * display_lock_stat(char * p);
/*__________________________________________________________________________
*/
static char * show_flock_help(char * pChar) {
  pChar += sprintf(pChar,"usage:\n");
  pChar += sprintf(pChar,"flock reset       : reset statistics\n");
  pChar += sprintf(pChar,"flock             : display statistics\n");  
  return pChar; 
}
void show_flock(char * argv[], uint32_t tcpRef, void *bufRef) {
  char *pChar = uma_dbg_get_buffer();
       
  if (argv[1] != NULL) {

    if (strcmp(argv[1],"reset")== 0) {
      reset_lock_stat();
      uma_dbg_send(tcpRef, bufRef, TRUE, "Reset done\n");    
      return;
    }

    pChar = show_flock_help(pChar);
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
    return;
  }
  display_lock_stat(uma_dbg_get_buffer());
  uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());      
}    
/*__________________________________________________________________________
*/  
#define SHOW_PROFILER_PROBE(probe) pChar += sprintf(pChar," %-12s | %15"PRIu64" | %9"PRIu64" | %18"PRIu64" | %15s |\n",\
                    #probe,\
                    gprofiler.rozofs_ll_##probe[P_COUNT],\
                    gprofiler.rozofs_ll_##probe[P_COUNT]?gprofiler.rozofs_ll_##probe[P_ELAPSE]/gprofiler.rozofs_ll_##probe[P_COUNT]:0,\
                    gprofiler.rozofs_ll_##probe[P_ELAPSE]," " );

#define SHOW_PROFILER_PROBE_BYTE(probe) pChar += sprintf(pChar," %-12s | %15"PRIu64" | %9"PRIu64" | %18"PRIu64" | %15"PRIu64" |\n",\
                    #probe,\
                    gprofiler.rozofs_ll_##probe[P_COUNT],\
                    gprofiler.rozofs_ll_##probe[P_COUNT]?gprofiler.rozofs_ll_##probe[P_ELAPSE]/gprofiler.rozofs_ll_##probe[P_COUNT]:0,\
                    gprofiler.rozofs_ll_##probe[P_ELAPSE],\
                    gprofiler.rozofs_ll_##probe[P_BYTES]);


#define RESET_PROFILER_PROBE(probe) \
{ \
         gprofiler.rozofs_ll_##probe[P_COUNT] = 0;\
         gprofiler.rozofs_ll_##probe[P_ELAPSE] = 0; \
}

#define RESET_PROFILER_PROBE_BYTE(probe) \
{ \
   RESET_PROFILER_PROBE(probe);\
   gprofiler.rozofs_ll_##probe[P_BYTES] = 0; \
}
static char * show_profiler_help(char * pChar) {
  pChar += sprintf(pChar,"usage:\n");
  pChar += sprintf(pChar,"profiler reset       : reset statistics\n");
  pChar += sprintf(pChar,"profiler             : display statistics\n");  
  return pChar; 
}
void show_profiler(char * argv[], uint32_t tcpRef, void *bufRef) {
    char *pChar = uma_dbg_get_buffer();

    time_t elapse;
    int days, hours, mins, secs;
    
    if (argv[1] != NULL)
    {
      if (strcmp(argv[1],"reset")==0) {
	RESET_PROFILER_PROBE(lookup);
	RESET_PROFILER_PROBE(forget);
	RESET_PROFILER_PROBE(getattr);
	RESET_PROFILER_PROBE(setattr);
	RESET_PROFILER_PROBE(readlink);
	RESET_PROFILER_PROBE(mknod);
	RESET_PROFILER_PROBE(mkdir);
	RESET_PROFILER_PROBE(unlink);
	RESET_PROFILER_PROBE(rmdir);
	RESET_PROFILER_PROBE(symlink);
	RESET_PROFILER_PROBE(rename);
	RESET_PROFILER_PROBE(open);
	RESET_PROFILER_PROBE(link);
	RESET_PROFILER_PROBE_BYTE(read);
	RESET_PROFILER_PROBE_BYTE(write);
	RESET_PROFILER_PROBE(flush);
	RESET_PROFILER_PROBE(release);
	RESET_PROFILER_PROBE(opendir);
	RESET_PROFILER_PROBE(readdir);
	RESET_PROFILER_PROBE(releasedir);
	RESET_PROFILER_PROBE(fsyncdir);
	RESET_PROFILER_PROBE(statfs);
	RESET_PROFILER_PROBE(setxattr);
	RESET_PROFILER_PROBE(getxattr);
	RESET_PROFILER_PROBE(listxattr);
	RESET_PROFILER_PROBE(removexattr);
	RESET_PROFILER_PROBE(access);
	RESET_PROFILER_PROBE(create);
	RESET_PROFILER_PROBE(getlk);
	RESET_PROFILER_PROBE(setlk);
	RESET_PROFILER_PROBE(setlk_int);
	RESET_PROFILER_PROBE(clearlkowner);      
	RESET_PROFILER_PROBE(ioctl);
	uma_dbg_send(tcpRef, bufRef, TRUE, "Reset Done\n");    
	return;
      }
      /*
      ** Help
      */
      pChar = show_profiler_help(pChar);
      uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
      return;
    }

    // Compute uptime for storaged process
    elapse = (int) (time(0) - gprofiler.uptime);
    days = (int) (elapse / 86400);
    hours = (int) ((elapse / 3600) - (days * 24));
    mins = (int) ((elapse / 60) - (days * 1440) - (hours * 60));
    secs = (int) (elapse % 60);


    pChar += sprintf(pChar, "GPROFILER version %s uptime =  %d days, %d:%d:%d\n", gprofiler.vers,days, hours, mins, secs);
    pChar += sprintf(pChar, " - ientry counter: %llu\n", (long long unsigned int) rozofs_ientries_count);
    pChar += sprintf(pChar, "   procedure  |     count       |  time(us) | cumulated time(us) |     bytes       |\n");
    pChar += sprintf(pChar, "--------------+-----------------+-----------+--------------------+-----------------+\n");
    SHOW_PROFILER_PROBE(lookup);
    SHOW_PROFILER_PROBE(forget);
    SHOW_PROFILER_PROBE(getattr);
    SHOW_PROFILER_PROBE(setattr);
    SHOW_PROFILER_PROBE(readlink);
    SHOW_PROFILER_PROBE(mknod);
    SHOW_PROFILER_PROBE(mkdir);
    SHOW_PROFILER_PROBE(unlink);
    SHOW_PROFILER_PROBE(rmdir);
    SHOW_PROFILER_PROBE(symlink);
    SHOW_PROFILER_PROBE(rename);
    SHOW_PROFILER_PROBE(open);
    SHOW_PROFILER_PROBE(link);
    SHOW_PROFILER_PROBE_BYTE(read);
    SHOW_PROFILER_PROBE_BYTE(write);
    SHOW_PROFILER_PROBE(flush);
    SHOW_PROFILER_PROBE(release);
    SHOW_PROFILER_PROBE(opendir);
    SHOW_PROFILER_PROBE(readdir);
    SHOW_PROFILER_PROBE(releasedir);
    SHOW_PROFILER_PROBE(fsyncdir);
    SHOW_PROFILER_PROBE(statfs);
    SHOW_PROFILER_PROBE(setxattr);
    SHOW_PROFILER_PROBE(getxattr);
    SHOW_PROFILER_PROBE(listxattr);
    SHOW_PROFILER_PROBE(removexattr);
    SHOW_PROFILER_PROBE(access);
    SHOW_PROFILER_PROBE(create);
    SHOW_PROFILER_PROBE(getlk);
    SHOW_PROFILER_PROBE(setlk);
    SHOW_PROFILER_PROBE(setlk_int);
    SHOW_PROFILER_PROBE(clearlkowner);
    SHOW_PROFILER_PROBE(ioctl);
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}
/*__________________________________________________________________________
*/
static char * rozofs_set_cache_help(char * pChar) {
  pChar += sprintf(pChar,"usage:\n");
  pChar += sprintf(pChar,"cache_set {0|1|2}\n");
  pChar += sprintf(pChar,"   0: no option on open/create\n");
  pChar += sprintf(pChar,"   1: direct_io\n");
  pChar += sprintf(pChar,"   2: keep_cache\n");
  return pChar; 
}
void rozofs_set_cache(char * argv[], uint32_t tcpRef, void *bufRef) 
{
   char *pChar = uma_dbg_get_buffer();
   int cache_mode;
   
   if (argv[1] ==NULL)
   {
    pChar = rozofs_set_cache_help(pChar);
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
    return;
   }
   errno = 0;
   cache_mode = (int) strtol(argv[1], (char **) NULL, 10);   
   if (errno != 0) {
      pChar += sprintf(pChar, "bad value %s\n",argv[1]);
      pChar = rozofs_set_cache_help(pChar);
      uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
      return;     
   } 
   if (cache_mode > 2)
   {
      pChar += sprintf(pChar, "bad value %s\n",argv[1]);
      pChar = rozofs_set_cache_help(pChar);
      uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
      return;     
   } 
   rozofs_cache_mode = cache_mode;
   uma_dbg_send(tcpRef, bufRef, TRUE, "Success\n");
}

/*__________________________________________________________________________
*/
static char * rozofs_set_fsmode_help(char * pChar) {
  pChar += sprintf(pChar,"usage:\n");
  pChar += sprintf(pChar,"fs_mode set {fs|block}   : set FS mode\n");
  return pChar; 
}
void rozofs_set_fsmode(char * argv[], uint32_t tcpRef, void *bufRef) 
{
  char * pChar = uma_dbg_get_buffer();
   
   if (argv[1] ==NULL)
   {
    pChar = rozofs_set_fsmode_help(pChar);
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
    return;
   }
   if      (strcmp(argv[1],"fs")==0)    rozofs_mode = 0;
   else if (strcmp(argv[1],"block")==0) rozofs_mode = 1;
   else {
      pChar += sprintf(pChar, "bad value %s\n",argv[1]);
      pChar = rozofs_set_fsmode_help(pChar);
      uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());   
      return;       
   }
   uma_dbg_send(tcpRef, bufRef, TRUE, "Success\n");
}
/*__________________________________________________________________________
*/

void show_exp_routing_table(char * argv[], uint32_t tcpRef, void *bufRef) {

    char *pChar = uma_dbg_get_buffer();
    
    expgw_display_all_exportd_routing_table(pChar);

    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}

/*__________________________________________________________________________
*/
void show_eid_exportd_assoc(char * argv[], uint32_t tcpRef, void *bufRef) {

    char *pChar = uma_dbg_get_buffer();
    
    expgw_display_all_eid(pChar);

    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}

/*__________________________________________________________________________
*/
static char * show_blockmode_cache_help(char * pChar) {
  pChar += sprintf(pChar,"usage:\n");
  pChar += sprintf(pChar,"blockmode_cache reset       : reset statistics\n");
  pChar += sprintf(pChar,"blockmode_cache flush       : flush block mode cache\n");  
  pChar += sprintf(pChar,"blockmode_cache enable      : enable block mode cache\n");  
  pChar += sprintf(pChar,"blockmode_cache disable     : disable block mode cache\n");  
  pChar += sprintf(pChar,"blockmode_cache             : display statistics\n");  
  return pChar; 
}  
void show_blockmode_cache(char * argv[], uint32_t tcpRef, void *bufRef) {


    char *pChar = uma_dbg_get_buffer();
    
    if (argv[1] != NULL)
    {
        if (strcmp(argv[1],"reset")==0) {
	  rozofs_mbcache_stats_clear();
	  uma_dbg_send(tcpRef, bufRef, TRUE, "Reset Done\n");    
	  return;
	}
        if (strcmp(argv[1],"flush")==0) {
	  rozofs_gcache_flush();
	  rozofs_mbcache_stats_clear();
	  uma_dbg_send(tcpRef, bufRef, TRUE, "Flush Done\n");    
	  return;
        }
        if (strcmp(argv[1],"enable")==0) {
	  if (rozofs_mbcache_enable_flag != ROZOFS_MBCACHE_ENABLE)
	  {
            rozofs_mbcache_enable();
            rozofs_mbcache_stats_clear();
            uma_dbg_send(tcpRef, bufRef, TRUE, "Mode Block cache is now enabled\n");    
	  }
	  else
	  {
            uma_dbg_send(tcpRef, bufRef, TRUE, "Mode Block cache is already enabled\n");    
	  }
	}  
        if (strcmp(argv[1],"disable")==0) {
	  if (rozofs_mbcache_enable_flag != ROZOFS_MBCACHE_DISABLE)
	  {
            rozofs_mbcache_stats_clear();
            rozofs_mbcache_disable();
            uma_dbg_send(tcpRef, bufRef, TRUE, "Mode Block cache is now disabled\n");    
	  }
	  else
	  {
            uma_dbg_send(tcpRef, bufRef, TRUE, "Mode Block cache is already disabled\n");    
	  }
	  return;
        }
	pChar = show_blockmode_cache_help(pChar);
	uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
	return;   	
    } 
    pChar +=sprintf(pChar,"cache state : %s\n", (rozofs_mbcache_enable_flag== ROZOFS_MBCACHE_ENABLE)?"Enabled":"Disabled"); 
    
    pChar = com_cache_show_cache_stats(pChar,rozofs_mbcache_cache_p,"Block mode cache");
    rozofs_mbcache_stats_display(pChar);

    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}

typedef struct _xmalloc_stats_t {
    uint64_t count;
    int size;
} xmalloc_stats_t;

#define XMALLOC_MAX_SIZE  512

extern xmalloc_stats_t *xmalloc_size_table_p;

void show_xmalloc(char * argv[], uint32_t tcpRef, void *bufRef) {
    char *pChar = uma_dbg_get_buffer();
    int i;
    xmalloc_stats_t *p;

    if (xmalloc_size_table_p == NULL) {
        pChar += sprintf(pChar, "xmalloc stats not available\n");
        uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
        return;

    }

    for (i = 0; i < 64; i++) {
        p = &xmalloc_size_table_p[i];
        if (p->size == 0) {
            break;
        }
        pChar += sprintf(pChar, "size %8.8u count %10.10llu \n", p->size, (long long unsigned int) p->count);
    }
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());

}
static struct fuse_lowlevel_ops rozofs_ll_operations = {
    .init = rozofs_ll_init,
    //.destroy = rozofs_ll_destroy,
    .lookup = rozofs_ll_lookup_nb, /** non blocking */
    .forget = rozofs_ll_forget, /** non blocking by construction */
    .getattr = rozofs_ll_getattr_nb, /** non blocking */
    .setattr = rozofs_ll_setattr_nb, /** non blocking */
    .readlink = rozofs_ll_readlink_nb, /** non blocking */
    .mknod = rozofs_ll_mknod_nb, /** non blocking */
    .mkdir = rozofs_ll_mkdir_nb, /** non blocking */
    .unlink = rozofs_ll_unlink_nb, /** non blocking */
    .rmdir = rozofs_ll_rmdir_nb, /** non blocking */
    .symlink = rozofs_ll_symlink_nb, /** non blocking */
    .rename = rozofs_ll_rename_nb, /** non blocking */
    .open = rozofs_ll_open_nb, /** non blocking */
    .link = rozofs_ll_link_nb, /** non blocking */
    .read = rozofs_ll_read_nb, /**non blocking */
    .write = rozofs_ll_write_nb, /**non blocking */
    .flush = rozofs_ll_flush_nb, /**non blocking */
    .release = rozofs_ll_release_nb, /**non blocking */
    //.opendir = rozofs_ll_opendir, /** non blocking by construction */
    .readdir = rozofs_ll_readdir_nb, /** non blocking */
    //.releasedir = rozofs_ll_releasedir, /** non blocking by construction */
    //.fsyncdir = rozofs_ll_fsyncdir, /** non blocking by construction */
    .statfs = rozofs_ll_statfs_nb, /** non blocking */
    .setxattr = rozofs_ll_setxattr_nb, /** non blocking */
    .getxattr = rozofs_ll_getxattr_nb, /** non blocking */
    .listxattr = rozofs_ll_listxattr_nb, /** non blocking */
    .removexattr = rozofs_ll_removexattr_nb, /** non blocking */
    //.access = rozofs_ll_access, /** non blocking by construction */
    .create = rozofs_ll_create_nb, /** non blocking */
    
    // POSIX lock to be activated thanks to -o posixlock option
    //.getlk = rozofs_ll_getlk_nb,
    //.setlk = rozofs_ll_setlk_nb,

    // BSD lock to be activated thanks to -o bsdlock option
    //.flock = rozofs_ll_flock_nb,

    //.bmap = rozofs_ll_bmap,
    //.ioctl = rozofs_ll_ioctl,
    //.poll = rozofs_ll_poll,
};

void rozofs_kill_one_storcli(int instance) {

    char cmd[1024];
    char *cmd_p = &cmd[0];
    cmd_p += sprintf(cmd_p, "%s %s %d", STORCLI_KILLER, mountpoint, instance);
    system(cmd);
}

void rozofs_start_one_storcli(int instance) {
    char cmd[1024];
    
    char *cmd_p = &cmd[0];
    cmd_p += sprintf(cmd_p, "%s ", STORCLI_STARTER);
    cmd_p += sprintf(cmd_p, "%s ", STORCLI_EXEC);
    cmd_p += sprintf(cmd_p, "-i %d ", instance);
    cmd_p += sprintf(cmd_p, "-H %s ", conf.host);
    cmd_p += sprintf(cmd_p, "-E %s ", conf.export);
    cmd_p += sprintf(cmd_p, "-M %s ", mountpoint);
    cmd_p += sprintf(cmd_p, "-D %d ", conf.dbg_port + instance);
    cmd_p += sprintf(cmd_p, "-R %d ", conf.instance);
    cmd_p += sprintf(cmd_p, "--nbcores %d ", conf.nb_cores);
    cmd_p += sprintf(cmd_p, "--shaper %d ", conf.shaper);
    cmd_p += sprintf(cmd_p, "-s %d ", ROZOFS_TMR_GET(TMR_STORAGE_PROGRAM));
    /*
    ** check if there is a share mem key
    */
    if (rozofs_storcli_shared_mem[instance-1].key != 0)
    {
      cmd_p += sprintf(cmd_p, "-k %d ",rozofs_storcli_shared_mem[instance-1].key);       
      cmd_p += sprintf(cmd_p, "-l %d ",rozofs_storcli_shared_mem[instance-1].buf_sz);       
      cmd_p += sprintf(cmd_p, "-c %d ",rozofs_storcli_shared_mem[instance-1].buf_count);       
    }
    cmd_p += sprintf(cmd_p, "&");

    info("start storcli (instance: %d, export host: %s, export path: %s, mountpoint: %s,"
            " profile port: %d, rozofs instance: %d, storage timeout: %d).",
            instance, conf.host, conf.export, mountpoint,
            conf.dbg_port + instance, conf.instance,
            ROZOFS_TMR_GET(TMR_STORAGE_PROGRAM));

    system(cmd);
}
void rozofs_kill_storcli() {
    int i;

    for (i = 1; i <= STORCLI_PER_FSMOUNT; i++) {
      rozofs_kill_one_storcli(i);
    }
}
void rozofs_start_storcli() {
	int i;

	rozofs_kill_storcli();

	i = stclbg_get_storcli_number();
	while (i) {
		rozofs_start_one_storcli(i);
		i--;
	}
}

int fuseloop(struct fuse_args *args, int fg) {
    int i = 0;
    int ret;
    int err;
    char *c;
    int piped[2];
    piped[0] = piped[1] = -1;
    char s;
    struct fuse_chan *ch;
    struct fuse_session *se;
    int sock;
    pthread_t profiling_thread;
    uint16_t profiling_port;
    int retry_count;
    char ppfile[NAME_MAX];
    int ppfd = -1;

    openlog("rozofsmount", LOG_PID, LOG_LOCAL0);

    struct timeval timeout_mproto;
    timeout_mproto.tv_sec = rozofs_tmr_get(TMR_EXPORT_PROGRAM);
    timeout_mproto.tv_usec = 0;

    for (retry_count = 5; retry_count > 0; retry_count--) {
        /* Initiate the connection to the export and get information
         * about exported filesystem */
        if (exportclt_initialize(
                &exportclt,
                conf.host,
                conf.export,
                conf.passwd,
                conf.buf_size * 1024,
                conf.min_read_size * 1024,
                conf.max_retry,
                timeout_mproto) == 0) break;

        sleep(2);
    }

    if (retry_count == 0) {

        fprintf(stderr,
                "rozofsmount failed for:\n" "export directory: %s\n"
                "export hostname: %s\n" "local mountpoint: %s\n" "error: %s\n"
                "See log for more information\n", conf.export, conf.host,
                mountpoint, strerror(errno));
        return 1;
    }

    /*
    ** Send the file lock reset request to remove old locks
    */
    {
    	epgw_lock_arg_t     arg;

    	arg.arg_gw.eid             = exportclt.eid;
    	arg.arg_gw.lock.client_ref = rozofs_client_hash;

    	ep_clear_client_file_lock_1(&arg, exportclt.rpcclt.client);
    }

    /* Initialize list and htables for inode_entries */
    list_init(&inode_entries);
    htable_initialize(&htable_inode, INODE_HSIZE, fuse_ino_hash, fuse_ino_cmp);
    htable_initialize(&htable_fid, PATH_HSIZE, fid_hash, fid_cmp);

    /* Put the root inode entry*/
    ientry_t *root = xmalloc(sizeof (ientry_t));
    memset(root, 0, sizeof (ientry_t));
    memcpy(root->fid, exportclt.rfid, sizeof (fid_t));
    root->inode = ROOT_INODE;
    root->db.size = 0;
    root->db.eof = 0;
    root->db.p = NULL;
    root->nlookup = 1;
    put_ientry(root);

    info("mounting - export: %s from : %s on: %s", conf.export, conf.host,
            mountpoint);

    if (fg == 0) {
        if (pipe(piped) < 0) {
            fprintf(stderr, "pipe error\n");
            return 1;
        }
        err = fork();
        if (err < 0) {
            fprintf(stderr, "fork error\n");
            return 1;
        } else if (err > 0) {
            // Parent process closes up output side of pipe
            close(piped[1]);
            err = read(piped[0], &s, 1);
            if (err == 0) {
                s = 1;
            }
            return s;
        }
        // Child process closes up input side of pipe
        close(piped[0]);
        s = 1;
    }
    if ((ch = fuse_mount(mountpoint, args)) == NULL) {
        fprintf(stderr, "error in fuse_mount\n");
        if (piped[1] >= 0) {
            if (write(piped[1], &s, 1) != 1) {
                fprintf(stderr, "pipe write error\n");
            }
            close(piped[1]);
        }
        return 1;
    }
    /*
    ** Are POSIX lock required ?
    */
    if (conf.posix_file_lock) {
      rozofs_ll_operations.getlk = rozofs_ll_getlk_nb;
      rozofs_ll_operations.setlk = rozofs_ll_setlk_nb;
    }
    /*
    ** Are BSD lock required ?
    */
    if (conf.bsd_file_lock) {
      rozofs_ll_operations.flock = rozofs_ll_flock_nb;
    }

    se = fuse_lowlevel_new(args, &rozofs_ll_operations,
            sizeof (rozofs_ll_operations), (void *) piped);

    if (se == NULL) {
        fuse_unmount(mountpoint, ch);
        fprintf(stderr, "error in fuse_lowlevel_new\n");
        usleep(100000); // time for print other error messages by FUSE
        if (piped[1] >= 0) {
            if (write(piped[1], &s, 1) != 1) {
                fprintf(stderr, "pipe write error\n");
            }
            close(piped[1]);
        }
        return 1;
    }

    fuse_session_add_chan(se, ch);

    if (fuse_set_signal_handlers(se) < 0) {
        fprintf(stderr, "error in fuse_set_signal_handlers\n");
        fuse_session_remove_chan(ch);
        fuse_session_destroy(se);
        fuse_unmount(mountpoint, ch);
        if (piped[1] >= 0) {
            if (write(piped[1], &s, 1) != 1) {
                fprintf(stderr, "pipe write error\n");
            }
            close(piped[1]);
        }
        return 1;
    }

    if (fg == 0) {
        setsid();
        setpgid(0, getpid());
        if ((i = open("/dev/null", O_RDWR, 0)) != -1) {
            (void) dup2(i, STDIN_FILENO);
            (void) dup2(i, STDOUT_FILENO);
            (void) dup2(i, STDERR_FILENO);
            if (i > 2)
                close(i);
        }
    }

    /* Creates one debug thread */

    pthread_t thread;
    rozofs_fuse_conf_t rozofs_fuse_conf;

    semForEver = malloc(sizeof (sem_t)); /* semaphore for blocking the main thread doing nothing */
    //    int ret;

    if (sem_init(semForEver, 0, 0) == -1) {
        severe("main() sem_init() problem : %s", strerror(errno));
    }

    /*
     ** Register these topics before start the rozofs_stat_start that will
     ** register other topic. Topic registration is not safe in multi-thread
     ** case
     */
    uma_dbg_addTopic("stclbg", show_stclbg);
    uma_dbg_addTopic("profiler", show_profiler);
    uma_dbg_addTopic("xmalloc", show_xmalloc);
    uma_dbg_addTopic("exp_route", show_exp_routing_table);
    uma_dbg_addTopic("exp_eid", show_eid_exportd_assoc);
    uma_dbg_addTopic("cache_set", rozofs_set_cache);
    uma_dbg_addTopic("fsmode_set", rozofs_set_fsmode);
    uma_dbg_addTopic("shared_mem", rozofs_shared_mem_display);
    uma_dbg_addTopic("blockmode_cache", show_blockmode_cache);
    uma_dbg_addTopic("data_cache", rozofs_gcache_show_cache_stats);
    uma_dbg_addTopic("start_config", show_start_config);
    uma_dbg_addTopic("rotateModulo", show_rotate_modulo);
    uma_dbg_addTopic("flock", show_flock);

    /*
    ** Initialize the number of write pending per fd
    ** and reset statistics
    */
    init_write_flush_stat(conf.max_write_pending);

    /**
    * init of the mode block cache
    */
    ret = rozofs_mbcache_cache_init(ROZOFS_MBCACHE_DISABLE);
    if (ret < 0)
    {
      severe("Cannot create the mode block cache, revert to non-caching mode");
    }
    /**
    * init of the common cache array
    */
    ret = rozofs_gcache_pool_init();
    if (ret < 0)
    {
      severe("Cannot create the global cache, revert to non-caching mode");
    }
    /*
    ** declare timer debug functions
    */
    rozofs_timer_conf_dbg_init();
    /*
    ** Check if the base port of rozodebug has been provided, if there is no value, set it to default
    */
    if (conf.dbg_port == 0) 
    {
      conf.dbg_port = rzdbg_default_base_port;    
    }
    else
    {
      rzdbg_default_base_port = conf.dbg_port;    
    }    
    
    if (conf.nb_cores == 0) 
    {
      conf.nb_cores = 2;    
    }     
    rozofs_fuse_conf.instance = (uint16_t) conf.instance;
    rozofs_fuse_conf.debug_port = (uint16_t)rzdbg_get_rozofsmount_port((uint16_t) conf.instance);
    rozofs_fuse_conf.nb_cores = (uint16_t) conf.nb_cores;
    conf.dbg_port = rozofs_fuse_conf.debug_port;
    rozofs_fuse_conf.se = se;
    rozofs_fuse_conf.ch = ch;
    rozofs_fuse_conf.exportclt = (void*) &exportclt;
    rozofs_fuse_conf.max_transactions = ROZOFSMOUNT_MAX_TX;

    if ((errno = pthread_create(&thread, NULL, (void*) rozofs_stat_start, &rozofs_fuse_conf)) != 0) {
        severe("can't create debug thread: %s", strerror(errno));
        return err;
    }

    /*
     * Start profiling server
     */
    gprofiler.uptime = time(0);
    strncpy((char *) gprofiler.vers, VERSION, 20);
    /* Find a free port */
    for (profiling_port = 52000; profiling_port < 53000; profiling_port++) {
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            severe("can't create socket: %s", strerror(errno));
            break;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(profiling_port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if ((bind(sock, (struct sockaddr *) &addr, sizeof (struct sockaddr_in))) != 0) {
            if (errno == EADDRINUSE)
                profiling_port++; /* Try next port */
            else {
                severe("can't bind socket: %s", strerror(errno));
                close(sock);
            }
        } else {
            shutdown(sock, 2);
            close(sock);
            break;
        }
    }

    if (profiling_port >= 60000) {
        severe("no free port for monitoring !");
    } else {
        if ((errno = pthread_create(&profiling_thread, NULL,
                (void*) rozofmount_profiling_thread_run,
                &profiling_port)) != 0) {
            severe("can't create monitoring thread: %s", strerror(errno));
        }
    }
    info("monitoring port: %d", profiling_port);

    /* try to create a flag file with port number */
    sprintf(ppfile, "%s%s_%d%s", DAEMON_PID_DIRECTORY, "rozofsmount",conf.instance, mountpoint);
    c = ppfile + strlen(DAEMON_PID_DIRECTORY);
    while (*c++) {
        if (*c == '/') *c = '.';
    }
    if ((ppfd = open(ppfile, O_RDWR | O_CREAT, 0640)) < 0) {
        severe("can't open profiling port file");
    } else {
        char str[10];
        sprintf(str, "%d\n", getpid());
		if ((write(ppfd, str, strlen(str))) != strlen(str)) {
			severe("can't write pid on flag file");
		}
        close(ppfd);
    }
    /*
    ** create the shared memory used by the storcli's
    */
    for (i = 0; i < STORCLI_PER_FSMOUNT; i++) {
       /*
       ** the size of the buffer is retrieved from the configuration. 1K is added for the management part of
       ** the RPC protocol. The key_instance of the shared memory is the concatenantion of the rozofsmount instance and
       ** storcli instance: (rozofsmount<<1 | storcli_instance) (assuming of max of 2 storclis per rozofsmount)
       */
       int key_instance = conf.instance<<STORCLI_PER_FSMOUNT_POWER2 | i;
       ret = rozofs_create_shared_memory(key_instance,i,rozofs_fuse_conf.max_transactions,(conf.buf_size*1024)+1024);
       if (ret < 0)
       {
         severe("Cannot create the shared memory for storcli %d\n",i);
       }
    }   
    /*
    ** start the storcli processes
    */ 
    rozofs_start_storcli();

    for (;;) {
        int ret;
        ret = sem_wait(semForEver);
        if (ret != 0) {
            severe("sem_wait: %s", strerror(errno));
            continue;

        }
        break;
    }

    rozofs_kill_storcli();

    fuse_remove_signal_handlers(se);
    fuse_session_remove_chan(ch);
    fuse_session_destroy(se);
    fuse_unmount(mountpoint, ch);
    exportclt_release(&exportclt);
    ientries_release();
    rozofs_layout_release();
    if (conf.export != NULL)
        free(conf.export);
    if (conf.host != NULL)
        free(conf.host);
    if (conf.passwd != NULL)
        free(conf.passwd);
    unlink(ppfile); // best effort

    return err ? 1 : 0;
}

void rozofs_allocate_flush_buf(int size_kB);

/** Check if a given RozoFS mountpoint is already mounted
 *
 * @param *mntpoint: mountpoint to check
 *
 * @return: 0 on success -1 otherwise
 */
int rozofs_mountpoint_check(const char * mntpoint) {

    struct mntent* mnt_entry = NULL;
    FILE* mnt_file_stream = NULL;
    char mountpoint_path[PATH_MAX];

    if (!realpath(mntpoint, mountpoint_path)) {
        fprintf(stderr, "bad mount point %s: %s\n", mntpoint, strerror(errno));
        return -1;
    }

    mnt_file_stream = setmntent(MOUNTED_FS_FILE_CHECK, "r");
    if (mnt_file_stream == NULL) {
        fprintf(stderr, "setmntent failed for file "MOUNTED_FS_FILE_CHECK":"
        " %s \n", strerror(errno));
        return -1;
    }

    while ((mnt_entry = getmntent(mnt_file_stream))) {

        if ((strcmp(mountpoint_path, mnt_entry->mnt_dir) == 0)
                && (strcmp(FSNAME, mnt_entry->mnt_fsname) == 0)) {
            fprintf(stderr,
                    "according to '"MOUNTED_FS_FILE_CHECK"', %s is already a"
                    " active mountpoint for a Rozo file system\n",
                    mountpoint_path);
            return -1;
        }
    }

    endmntent(mnt_file_stream);

    return 0;
}

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int fg = 0;
    int res;
    struct rlimit core_limit;

    memset(&conf, 0, sizeof (conf));
    /*
    ** init of the shared memory data structure
    */
    rozofs_init_shared_memory();
    /*
    ** init of the timer configuration
    */
    rozofs_tmr_init_configuration();

    conf.max_retry = 50;
    conf.buf_size = 0;
    conf.min_read_size = 0;
    conf.max_write_pending = ROZOFSMOUNT_MAX_TX; /* No limit */ 
    conf.attr_timeout = 10;
    conf.entry_timeout = 10;
    conf.nbstorcli = 0;
    conf.shaper = 1; // Default traffic shaper value
    conf.rotate = 0;
    conf.posix_file_lock = 0; // No posix file lock until explicitly activated  man 2 fcntl)
    conf.bsd_file_lock = 0;   // No BSD file lock until explicitly activated    man 2 flock)
    if (fuse_opt_parse(&args, &conf, rozofs_opts, myfs_opt_proc) < 0) {
        exit(1);
    }

    if (conf.host == NULL) {
        conf.host = strdup("rozofsexport");
    }

    if (strlen(conf.host) >= ROZOFS_HOSTNAME_MAX) {
        fprintf(stderr,
                "The length of export host must be lower than %d\n",
                ROZOFS_HOSTNAME_MAX);
    }

    if (conf.export == NULL) {
        conf.export = strdup("/srv/rozofs/exports/export");
    }

    if (conf.passwd == NULL) {
        conf.passwd = strdup("none");
    }

    if (conf.buf_size == 0) {
        conf.buf_size = 256;
    }
    if (conf.buf_size < 128) {
        fprintf(stderr,
                "write cache size too low (%u KiB) - increased to 128 KiB\n",
                conf.buf_size);
        conf.buf_size = 128;
    }
    if (conf.buf_size > 256) {
        fprintf(stderr,
                "write cache size too big (%u KiB) - decreased to 256 KiB\n",
                conf.buf_size);
        conf.buf_size = 256;
    }
    /* Bufsize must be a multiple of the block size */
    if ((conf.buf_size % (ROZOFS_BSIZE/1024)) != 0) {
      conf.buf_size = ((conf.buf_size / (ROZOFS_BSIZE/1024))+1) * (ROZOFS_BSIZE/1024);
    }
    
    if (conf.min_read_size == 0) {
      conf.min_read_size = conf.buf_size;
    }
    if (conf.min_read_size > conf.buf_size) {
      conf.min_read_size = conf.buf_size;
    }
    /* Bufsize must be a multiple of the block size */
    if ((conf.min_read_size % (ROZOFS_BSIZE/1024)) != 0) {
      conf.min_read_size = ((conf.min_read_size / (ROZOFS_BSIZE/1024))+1) * (ROZOFS_BSIZE/1024);
    }    
    
    if (conf.nbstorcli != 0) {
      if (stclbg_set_storcli_number(conf.nbstorcli) < 0) {
          fprintf(stderr,
                  "invalid rozofsnbstorcli parameter (%d) allowed range is [1..%d]\n",
                  conf.nbstorcli,STORCLI_PER_FSMOUNT);
      }
    }
    
    /* Initialize the rotation modulo on distribution for read request */
    rozofs_rotation_read_modulo = conf.rotate;
    
    /*
    ** Compute the identifier of the client from host and instance id 
    */
    {
        char hostName[256];
        hostName[0] = 0;
        gethostname(hostName, 256);
        rozofs_client_hash = rozofs_client_hash_compute(hostName, conf.instance);
    }
    /*
    ** allocate the common flush buffer
    */
    rozofs_allocate_flush_buf(conf.buf_size);

    // Set timeout for exportd requests
    if (conf.export_timeout != 0) {
        if (rozofs_tmr_configure(TMR_EXPORT_PROGRAM,conf.export_timeout)< 0)
        {
          fprintf(stderr,
                "timeout for exportd requests is out of range: revert to default setting");
        }
    }

    if (conf.storage_timeout != 0) {
        if (rozofs_tmr_configure(TMR_STORAGE_PROGRAM,conf.storage_timeout)< 0)
        {
          fprintf(stderr,
                "timeout for storaged requests is out of range: revert to default setting");
        }
    }

    if (conf.storcli_timeout != 0) {
        if (rozofs_tmr_configure(TMR_STORCLI_PROGRAM,conf.storcli_timeout)< 0)
        {
          fprintf(stderr,
                "timeout for storcli requests is out of range: revert to default setting");
        }
    }
    if (conf.cache_mode > 2) {

          fprintf(stderr,
                "cache mode out of range: revert to default setting");
    }

    if (conf.fs_mode > 1) {

          fprintf(stderr,
                "rozofs mode out of range: revert to default setting");
          conf.fs_mode = 0;
    }
    rozofs_mode = conf.fs_mode;
    
    if (conf.attr_timeout != 10) {
        if (rozofs_tmr_configure(TMR_FUSE_ATTR_CACHE,conf.attr_timeout) < 0)
        {
          fprintf(stderr,
                "timeout for which file/directory attributes are cached is out"
                  " of range: revert to default setting");
        }
    }
    
    if (conf.entry_timeout != 10) {
        if (rozofs_tmr_configure(TMR_FUSE_ENTRY_CACHE,conf.entry_timeout) < 0)
        {
          fprintf(stderr,
                "timeout for which name lookups will be cached is out of range:"
                  " revert to default setting");
        }
    }

    if (fuse_version() < 28) {
        if (fuse_opt_add_arg(&args, "-o" FUSE27_DEFAULT_OPTIONS) == -1) {
            fprintf(stderr, "fuse_opt_add_arg failed\n");
            return 1;
        }
    } else {
        if (fuse_opt_add_arg(&args, "-o" FUSE28_DEFAULT_OPTIONS) == -1) {
            fprintf(stderr, "fuse_opt_add_arg failed\n");
            return 1;
        }
    }

    if (fuse_parse_cmdline(&args, &mountpoint, NULL, &fg) == -1) {
        fprintf(stderr, "see: %s -h for help\n", argv[0]);
        return 1;
    }

    if (!mountpoint) {
        fprintf(stderr, "no mount point\nsee: %s -h for help\n", argv[0]);
        return 1;
    }

    // Check the mountpoint
    if (rozofs_mountpoint_check(mountpoint) != 0) {
        return 1;
    }

    /*
    ** assert the cache mode
    */
    if (conf.fs_mode == 0) {

        if (conf.cache_mode >= 2) {
            rozofs_cache_mode = 0;
        } else {
            rozofs_cache_mode = conf.cache_mode;
        }
    } else {

        if (conf.cache_mode > 2) {
            rozofs_cache_mode = 0;
        } else {
            rozofs_cache_mode = conf.cache_mode;
        }
    }
    
    // Change the value of maximum size of core file
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &core_limit) < 0) {
        warning("Failed to change maximum size of core file: %s",
                strerror(errno));
    }

    res = fuseloop(&args, fg);

    fuse_opt_free_args(&args);
    return res;
}
