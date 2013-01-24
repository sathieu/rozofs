/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "sproto.h"
#include <rozofs/rozofs.h>

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

void *
sp_null_1(void *argp, CLIENT *clnt)
{
	static char clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, SP_NULL,
		(xdrproc_t) xdr_void, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&clnt_res);
}

sp_write_ret_t *
sp_write_1(sp_write_arg_t *argp, CLIENT *clnt)
{
	static sp_write_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, SP_WRITE,
		(xdrproc_t) xdr_sp_write_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_sp_write_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

sp_read_ret_t *
sp_read_1(sp_read_arg_t *argp, CLIENT *clnt)
{
	static sp_read_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, SP_READ,
		(xdrproc_t) xdr_sp_read_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_sp_read_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

sp_status_ret_t *
sp_truncate_1(sp_truncate_arg_t *argp, CLIENT *clnt)
{
	static sp_status_ret_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, SP_TRUNCATE,
		(xdrproc_t) xdr_sp_truncate_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_sp_status_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
