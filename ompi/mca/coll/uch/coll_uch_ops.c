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

#define COLL_UCH_CHECK(_call) do {              \
        if (UCC_OK != (_call)) {                \
            goto fallback;                      \
        }                                       \
    } while(0)

static inline int coll_uch_req_wait(uch_request_h req) {
    while (UCC_INPROGRESS == uch_test(req)) {
       opal_progress();
    }
    return uch_request_free(req);
}

int mca_coll_uch_allreduce(const void *sbuf, void *rbuf, int count, struct ompi_datatype_t *dtype,
                           struct ompi_op_t *op, struct ompi_communicator_t *comm,
                           mca_coll_base_module_t *module)
{
    uch_request_h req;
    ucc_dt_t ucc_dt;
    ucc_op_t ucc_op;
    mca_coll_uch_module_t *uch_module = (mca_coll_uch_module_t*)module;

    UCH_VERBOSE(20,"RUNNING UCH ALLREDUCE");
    ucc_dt = ompi_dtype_to_ucc_dtype(dtype);
        ucc_op = ompi_op_to_ucc_op(op);
    if (OPAL_UNLIKELY(UCC_DT_UNSUPPORTED == ucc_dt || UCC_OP_UNSUPPORTED == ucc_op)) {
        UCH_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback allreduce;",
                     dtype->super.name);
        goto fallback;
    }

    COLL_UCH_CHECK(uch_allreduce_init((void *)sbuf, rbuf, count, ucc_dt,
                                      ucc_op, uch_module->uch_comm, &req));
    COLL_UCH_CHECK(uch_start(req));
    COLL_UCH_CHECK(coll_uch_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCH_VERBOSE(20,"RUNNING FALLBACK ALLREDUCE");
    return uch_module->previous_allreduce(sbuf, rbuf, count, dtype, op,
                                          comm, uch_module->previous_allreduce_module);
}

int mca_coll_uch_barrier(struct ompi_communicator_t *comm,
                         mca_coll_base_module_t *module)
{
    uch_request_h req;
    mca_coll_uch_module_t *uch_module = (mca_coll_uch_module_t*)module;

    UCH_VERBOSE(20,"RUNNING UCH BARRIER");
    COLL_UCH_CHECK(uch_barrier_init(uch_module->uch_comm, &req));
    COLL_UCH_CHECK(uch_start(req));
    COLL_UCH_CHECK(coll_uch_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCH_VERBOSE(20,"RUNNING FALLBACK BARRIER");
    return uch_module->previous_barrier(comm, uch_module->previous_barrier_module);
}
