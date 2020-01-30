/**
  Copyright (c) 2020      Mellanox Technologies. All rights reserved.
  $COPYRIGHT$

  Additional copyrights may follow

  $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"
#include "coll_uch.h"
#include "coll_uch_dtypes.h"


int mca_coll_uch_allreduce(const void *sbuf, void *rbuf, int count, struct ompi_datatype_t *dtype,
                           struct ompi_op_t *op, struct ompi_communicator_t *comm,
                           mca_coll_base_module_t *module)
{
    ucc_dt_t ucc_dt;
    ucc_op_t ucc_op;
    int rc;
    UCH_VERBOSE(20,"RUNNING UCH ALLREDUCE");
    mca_coll_uch_module_t *uch_module = (mca_coll_uch_module_t*)module;
    ucc_dt = ompi_dtype_to_ucc_dtype(dtype);
        ucc_op = ompi_op_to_ucc_op(op);
    if (OPAL_UNLIKELY(UCC_DT_UNSUPPORTED == ucc_dt || UCC_OP_UNSUPPORTED == ucc_op)) {
        UCH_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback allreduce;",
                     dtype->super.name);
        rc = uch_module->previous_allreduce(sbuf, rbuf, count, dtype, op,
                                             comm, uch_module->previous_allreduce_module);
        return rc;
    }

    rc = uch_allreduce((void *)sbuf, rbuf, count, ucc_dt, ucc_op, uch_module->uch_comm);
    if (UCC_OK != rc) {
        UCH_VERBOSE(20,"RUNNING FALLBACK ALLREDUCE");
        rc = uch_module->previous_allreduce(sbuf, rbuf, count, dtype, op,
                                             comm, uch_module->previous_allreduce_module);
    }
    return rc;
}
