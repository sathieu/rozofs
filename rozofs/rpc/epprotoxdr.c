/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "epproto.h"
#include <rozofs/rozofs.h>

bool_t
xdr_epp_status_t (XDR *xdrs, epp_status_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_epp_status_ret_t (XDR *xdrs, epp_status_ret_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_epp_status_t (xdrs, &objp->status))
		 return FALSE;
	switch (objp->status) {
	case EPP_FAILURE:
		 if (!xdr_int (xdrs, &objp->epp_status_ret_t_u.error))
			 return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}

bool_t
xdr_epp_estat_t (XDR *xdrs, epp_estat_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_uint32_t (xdrs, &objp->eid))
		 return FALSE;
	 if (!xdr_uint32_t (xdrs, &objp->vid))
		 return FALSE;
	 if (!xdr_uint16_t (xdrs, &objp->bsize))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->blocks))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->bfree))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->files))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->ffree))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_epp_sstat_t (XDR *xdrs, epp_sstat_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_uint16_t (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_uint16_t (xdrs, &objp->sid))
		 return FALSE;
	 if (!xdr_uint8_t (xdrs, &objp->status))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->size))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->free))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_epp_vstat_t (XDR *xdrs, epp_vstat_t *objp)
{
	//register int32_t *buf;

	//int i;
	 if (!xdr_uint16_t (xdrs, &objp->vid))
		 return FALSE;
	 if (!xdr_uint16_t (xdrs, &objp->bsize))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->bfree))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->blocks))
		 return FALSE;
	 if (!xdr_uint32_t (xdrs, &objp->nb_storages))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->sstats, 2048,
		sizeof (epp_sstat_t), (xdrproc_t) xdr_epp_sstat_t))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_epp_profiler_t (XDR *xdrs, epp_profiler_t *objp)
{
	//register int32_t *buf;

	//int i;
	 if (!xdr_uint64_t (xdrs, &objp->uptime))
		 return FALSE;
	 if (!xdr_uint64_t (xdrs, &objp->now))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->vers, 20,
		sizeof (uint8_t), (xdrproc_t) xdr_uint8_t))
		 return FALSE;
	 if (!xdr_uint32_t (xdrs, &objp->nb_volumes))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->vstats, 16,
		sizeof (epp_vstat_t), (xdrproc_t) xdr_epp_vstat_t))
		 return FALSE;
	 if (!xdr_uint32_t (xdrs, &objp->nb_exports))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->estats, 2048,
		sizeof (epp_estat_t), (xdrproc_t) xdr_epp_estat_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_mount, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_umount, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_statfs, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_lookup, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_getattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_setattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_readlink, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_mknod, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_mkdir, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_unlink, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_rmdir, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_symlink, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_rename, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_readdir, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_read_block, 3,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_write_block, 3,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_link, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_setxattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_getxattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_removexattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_listxattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_lv1_resolve_entry, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_lv2_resolve_path, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_lookup_fid, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_update_files, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_update_blocks, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_stat, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_lookup, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_getattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_setattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_link, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_mknod, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_mkdir, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_unlink, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_rmdir, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_symlink, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_readlink, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_rename, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_read, 3,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_read_block, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_write_block, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_setxattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_getxattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_removexattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_listxattr, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_readdir, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->lv2_cache_put, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->lv2_cache_get, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->lv2_cache_del, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->volume_balance, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->volume_distribute, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->volume_stat, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mdir_open, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mdir_close, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mdir_read_attributes, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mdir_write_attributes, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mreg_open, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mreg_close, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mreg_read_attributes, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mreg_write_attributes, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mreg_read_dist, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mreg_write_dist, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mslnk_open, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mslnk_close, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mslnk_read_attributes, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mslnk_write_attributes, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mslnk_read_link, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->mslnk_write_link, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->get_mdirentry, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->put_mdirentry, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->del_mdirentry, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->list_mdirentries, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->gw_invalidate, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->gw_invalidate_all, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->gw_configuration, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->gw_poll, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_configuration, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_conf_gateway, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_poll, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_clearclient_flock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_clearowner_flock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_set_file_lock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_get_file_lock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->export_poll_file_lock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_clearclient_flock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_clearowner_flock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_set_file_lock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_get_file_lock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	 if (!xdr_vector (xdrs, (char *)objp->ep_poll_file_lock, 2,
		sizeof (uint64_t), (xdrproc_t) xdr_uint64_t))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_epp_profiler_ret_t (XDR *xdrs, epp_profiler_ret_t *objp)
{
	//register int32_t *buf;

	 if (!xdr_epp_status_t (xdrs, &objp->status))
		 return FALSE;
	switch (objp->status) {
	case EPP_SUCCESS:
		 if (!xdr_epp_profiler_t (xdrs, &objp->epp_profiler_ret_t_u.profiler))
			 return FALSE;
		break;
	case EPP_FAILURE:
		 if (!xdr_int (xdrs, &objp->epp_profiler_ret_t_u.error))
			 return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}
