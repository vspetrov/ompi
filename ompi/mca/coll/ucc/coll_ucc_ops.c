/**
  Copyright (c) 2020      Mellanox Technologies. All rights reserved.
  $COPYRIGHT$

  Additional copyrights may follow

  $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"
#include "coll_ucc.h"
#include "coll_ucc_dtypes.h"

#define COLL_UCC_CHECK(_call) do {              \
        if (UCC_OK != (_call)) {                \
            goto fallback;                      \
        }                                       \
    } while(0)

static inline ucc_status_t coll_ucc_req_wait(ucc_coll_req_h req) {
    ucc_status_t status;
    while (UCC_OK != (status = ucc_collective_test(req))) {
        if (status < 0) {
            UCC_ERROR("ucc_collective_test failed: %s",
                      ucc_status_string(status));
            return status;
        }
        ucc_context_progress(mca_coll_ucc_component.ucc_context);
        opal_progress();
    }
    return ucc_collective_finalize(req);
}

int mca_coll_ucc_barrier(struct ompi_communicator_t *comm,
                         mca_coll_base_module_t *module)
{
    ucc_coll_req_h req;
    mca_coll_ucc_module_t *ucc_module = (mca_coll_ucc_module_t*)module;
    UCC_VERBOSE(3, "running ucc barrier");

    ucc_coll_op_args_t coll = {
        .mask      = UCC_COLL_ARG_FIELD_COLL_TYPE,
        .coll_type = UCC_COLL_TYPE_BARRIER
    };

    COLL_UCC_CHECK(ucc_collective_init(&coll, &req, ucc_module->ucc_team));
    COLL_UCC_CHECK(ucc_collective_post(req));
    COLL_UCC_CHECK(coll_ucc_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCC_VERBOSE(3, "running fallback barrier");
    return ucc_module->previous_barrier(comm, ucc_module->previous_barrier_module);
}

#if 0
int mca_coll_ucc_allreduce(const void *sbuf, void *rbuf, int count, struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op, struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    mca_coll_ucc_module_t *ucc_module = (mca_coll_ucc_module_t*)module;
    ucc_coll_req_h req;
    ucc_dt_t       ucc_dt;
    ucc_op_t       ucc_op;
    size_t          dt_size;

    UCC_VERBOSE(3, "running ucc allreduce");
    ucc_dt = ompi_dtype_to_ucc_dtype(dtype);
        ucc_op = ompi_op_to_ucc_op(op);
    if (OPAL_UNLIKELY(UCC_DT_UNSUPPORTED == ucc_dt || UCC_OP_UNSUPPORTED == ucc_op)) {
        UCC_VERBOSE(5, "ompi_datatype is not supported: dtype = %s; calling fallback allreduce;",
                     dtype->super.name);
        goto fallback;
    }
    opal_datatype_type_size(&dtype->super, &dt_size);
    ucc_coll_op_args_t coll = {
        .coll_type = UCC_ALLREDUCE,
        .buffer_info = {
            .src_buffer = (void*)sbuf,
            .dst_buffer = rbuf,
            .len        = count*dt_size,
        },
        .reduce_info = {
            .dt = ucc_dt,
            .op = ucc_op,
            .count = count,
        },
        .alg.set_by_user = 0,
    };

    COLL_UCC_CHECK(ucc_collective_init(&coll, &req, ucc_module->ucc_team));
    COLL_UCC_CHECK(ucc_collective_post(req));
    COLL_UCC_CHECK(coll_ucc_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCC_VERBOSE(3, "running fallback allreduce");
    return ucc_module->previous_allreduce(sbuf, rbuf, count, dtype, op,
                                          comm, ucc_module->previous_allreduce_module);
}

int mca_coll_ucc_reduce(const void *sbuf, void *rbuf, int count, struct ompi_datatype_t *dtype,
                         struct ompi_op_t *op, int root, struct ompi_communicator_t *comm,
                         mca_coll_base_module_t *module)
{
    mca_coll_ucc_module_t *ucc_module = (mca_coll_ucc_module_t*)module;
    ucc_coll_req_h req;
    ucc_dt_t       ucc_dt;
    ucc_op_t       ucc_op;
    size_t          dt_size;

    UCC_VERBOSE(3, "running ucc reduce");
    ucc_dt = ompi_dtype_to_ucc_dtype(dtype);
        ucc_op = ompi_op_to_ucc_op(op);
    if (OPAL_UNLIKELY(UCC_DT_UNSUPPORTED == ucc_dt || UCC_OP_UNSUPPORTED == ucc_op)) {
        UCC_VERBOSE(5, "ompi_datatype is not supported: dtype = %s; calling fallback allreduce;",
                     dtype->super.name);
        goto fallback;
    }
    opal_datatype_type_size(&dtype->super, &dt_size);
    ucc_coll_op_args_t coll = {
        .coll_type = UCC_REDUCE,
        .buffer_info = {
            .src_buffer = (void*)sbuf,
            .dst_buffer = rbuf,
            .len        = count*dt_size,
        },
        .reduce_info = {
            .dt = ucc_dt,
            .op = ucc_op,
            .count = count,
        },
        .root            = root,
        .alg.set_by_user = 0,
    };

    COLL_UCC_CHECK(ucc_collective_init(&coll, &req, ucc_module->ucc_team));
    COLL_UCC_CHECK(ucc_collective_post(req));
    COLL_UCC_CHECK(coll_ucc_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCC_VERBOSE(3, "running fallback allreduce");
    return ucc_module->previous_reduce(sbuf, rbuf, count, dtype, op, root,
                                        comm, ucc_module->previous_reduce_module);
}


int mca_coll_ucc_bcast(void *buf, int count, struct ompi_datatype_t *dtype,
                       int root, struct ompi_communicator_t *comm,
                       mca_coll_base_module_t *module)
{
    mca_coll_ucc_module_t *ucc_module = (mca_coll_ucc_module_t*)module;
    ucc_coll_req_h req;
    size_t          dt_size;
    UCC_VERBOSE(3, "running ucc bcast");
    if (!ompi_datatype_is_contiguous_memory_layout(dtype, count)) {
        goto fallback;
    }
    opal_datatype_type_size(&dtype->super, &dt_size);

    ucc_coll_op_args_t coll = {
        .coll_type = UCC_BCAST,
        .root = root,
        .buffer_info = {
            .src_buffer = buf,
            .dst_buffer = buf,
            .len        = count*dt_size,
        },
        .alg.set_by_user = 0,
    };

    COLL_UCC_CHECK(ucc_collective_init(&coll, &req, ucc_module->ucc_team));
    COLL_UCC_CHECK(ucc_collective_post(req));
    COLL_UCC_CHECK(coll_ucc_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCC_VERBOSE(3, "running fallback bcast");
    return ucc_module->previous_bcast(buf, count, dtype, root,
                                       comm, ucc_module->previous_bcast_module);
}


int mca_coll_ucc_alltoall(const void *sbuf, int scount, struct ompi_datatype_t *sdtype,
                           void* rbuf, int rcount, struct ompi_datatype_t *rdtype,
                           struct ompi_communicator_t *comm,
                           mca_coll_base_module_t *module)
{
    mca_coll_ucc_module_t *ucc_module = (mca_coll_ucc_module_t*)module;
    ucc_coll_req_h req;
    size_t          dt_size;
    UCC_VERBOSE(3, "running ucc alltoall");
    if (!ompi_datatype_is_contiguous_memory_layout(sdtype, scount) ||
        !ompi_datatype_is_contiguous_memory_layout(rdtype, rcount)) {
        goto fallback;
    }

    opal_datatype_type_size(&sdtype->super, &dt_size);

    ucc_coll_op_args_t coll = {
        .coll_type = UCC_ALLTOALL,
        .buffer_info = {
            .src_buffer = sbuf,
            .dst_buffer = rbuf,
            .len        = scount*dt_size,
        },
        .alg.set_by_user = 0,
    };

    COLL_UCC_CHECK(ucc_collective_init(&coll, &req, ucc_module->ucc_team));
    COLL_UCC_CHECK(ucc_collective_post(req));
    COLL_UCC_CHECK(coll_ucc_req_wait(req));
    return OMPI_SUCCESS;
fallback:
    UCC_VERBOSE(3, "running fallback alltoall");
    return ucc_module->previous_alltoall(sbuf, scount, sdtype, rbuf, rcount, rdtype,
                                          comm, ucc_module->previous_alltoall_module);
}
#endif
