/**
 * Copyright (c) 2020 Mellanox Technologies. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#include "coll_ucc.h"
#include "coll_ucc_dtypes.h"
#include "ompi/mca/coll/base/coll_tags.h"
#include "ompi/mca/pml/pml.h"

#define OBJ_RELEASE_IF_NOT_NULL( obj ) if( NULL != (obj) ) OBJ_RELEASE( obj );

static int ucc_comm_attr_keyval;
/*
 * Initial query function that is invoked during MPI_INIT, allowing
 * this module to indicate what level of thread support it provides.
 */
int mca_coll_ucc_init_query(bool enable_progress_threads, bool enable_mpi_threads)
{
    return OMPI_SUCCESS;
}

static void mca_coll_ucc_module_clear(mca_coll_ucc_module_t *ucc_module)
{
    ucc_module->ucc_team           = NULL;
    ucc_module->previous_allreduce = NULL;
    ucc_module->previous_reduce    = NULL;
    ucc_module->previous_barrier   = NULL;
    ucc_module->previous_bcast     = NULL;
    ucc_module->previous_alltoall  = NULL;
}

static void mca_coll_ucc_module_construct(mca_coll_ucc_module_t *ucc_module)
{
    mca_coll_ucc_module_clear(ucc_module);
}

int mca_coll_ucc_progress(void)
{
    ucc_context_progress(mca_coll_ucc_component.ucc_context);
    return OPAL_SUCCESS;
}

static void mca_coll_ucc_module_destruct(mca_coll_ucc_module_t *ucc_module)
{
    if (ucc_module->comm == &ompi_mpi_comm_world.comm){
        if (OMPI_SUCCESS != ompi_attr_free_keyval(COMM_ATTR, &ucc_comm_attr_keyval, 0)) {
            UCC_ERROR("ucc ompi_attr_free_keyval failed");
        }
    }
    OBJ_RELEASE_IF_NOT_NULL(ucc_module->previous_allreduce_module);
    OBJ_RELEASE_IF_NOT_NULL(ucc_module->previous_reduce_module);
    OBJ_RELEASE_IF_NOT_NULL(ucc_module->previous_barrier_module);
    OBJ_RELEASE_IF_NOT_NULL(ucc_module->previous_bcast_module);
    OBJ_RELEASE_IF_NOT_NULL(ucc_module->previous_alltoall_module);
    mca_coll_ucc_module_clear(ucc_module);
}

#define SAVE_PREV_COLL_API(__api) do {                                                       \
        ucc_module->previous_ ## __api            = comm->c_coll->coll_ ## __api;            \
        ucc_module->previous_ ## __api ## _module = comm->c_coll->coll_ ## __api ## _module; \
        if (!comm->c_coll->coll_ ## __api || !comm->c_coll->coll_ ## __api ## _module) {     \
            return OMPI_ERROR;                                                               \
        }                                                                                    \
        OBJ_RETAIN(ucc_module->previous_ ## __api ## _module);                               \
    } while(0)

static int mca_coll_ucc_save_coll_handlers(mca_coll_ucc_module_t *ucc_module)
{
    ompi_communicator_t *comm = ucc_module->comm;
    SAVE_PREV_COLL_API(allreduce);
    SAVE_PREV_COLL_API(reduce);
    SAVE_PREV_COLL_API(barrier);
    SAVE_PREV_COLL_API(bcast);
    SAVE_PREV_COLL_API(alltoall);
    return OMPI_SUCCESS;
}

/*
** Communicator free callback
*/
static int ucc_comm_attr_del_fn(MPI_Comm comm, int keyval, void *attr_val, void *extra)
{

    mca_coll_ucc_module_t *ucc_module = (mca_coll_ucc_module_t*) attr_val;
    ucc_team_destroy(ucc_module->ucc_team);
    if (ucc_module->comm == &ompi_mpi_comm_world.comm) {
        if (mca_coll_ucc_component.libucc_initialized) {
            UCC_VERBOSE(1,"finalizing ucc library");
            opal_progress_unregister(mca_coll_ucc_progress);
            ucc_context_destroy(mca_coll_ucc_component.ucc_context);
            ucc_finalize(mca_coll_ucc_component.ucc_lib);
        }
    }
    return OMPI_SUCCESS;
}

typedef struct oob_allgather_req{
    void           *sbuf;
    void           *rbuf;
    void           *oob_coll_ctx;
    size_t          msglen;
    int             iter;
    ompi_request_t *reqs[2];
} oob_allgather_req_t;

static ucc_status_t oob_allgather_test(void *req)
{
    oob_allgather_req_t *oob_req = (oob_allgather_req_t*)req;
    ompi_communicator_t *comm    = (ompi_communicator_t *)oob_req->oob_coll_ctx;
    char                *tmpsend = NULL;
    char                *tmprecv = NULL;
    size_t               msglen  = oob_req->msglen;
    int                  probe_count = 5;
    int rank, size, sendto, recvfrom, recvdatafrom,
        senddatafrom, completed, probe;

    size = ompi_comm_size(comm);
    rank = ompi_comm_rank(comm);
    if (oob_req->iter == 0) {
        tmprecv = (char*) oob_req->rbuf + (ptrdiff_t)rank * (ptrdiff_t)msglen;
        memcpy(tmprecv, oob_req->sbuf, msglen);
    }
    sendto   = (rank + 1) % size;
    recvfrom = (rank - 1 + size) % size;
    for (; oob_req->iter < size - 1; oob_req->iter++) {
        if (oob_req->iter > 0) {
            probe = 0;
            do {
                ompi_request_test_all(2, oob_req->reqs, &completed, MPI_STATUS_IGNORE);
                probe++;
            } while (!completed && probe < probe_count);
            if (!completed) {
                return UCC_INPROGRESS;
            }
        }
        recvdatafrom = (rank - oob_req->iter - 1 + size) % size;
        senddatafrom = (rank - oob_req->iter + size) % size;
        tmprecv = (char*)oob_req->rbuf + (ptrdiff_t)recvdatafrom * (ptrdiff_t)msglen;
        tmpsend = (char*)oob_req->rbuf + (ptrdiff_t)senddatafrom * (ptrdiff_t)msglen;
        MCA_PML_CALL(isend(tmpsend, msglen, MPI_BYTE, sendto, MCA_COLL_BASE_TAG_UCC,
                           MCA_PML_BASE_SEND_STANDARD, comm, &oob_req->reqs[0]));
        MCA_PML_CALL(irecv(tmprecv, msglen, MPI_BYTE, recvfrom,
                           MCA_COLL_BASE_TAG_UCC, comm, &oob_req->reqs[1]));
    }
    probe = 0;
    do {
        ompi_request_test_all(2, oob_req->reqs, &completed, MPI_STATUS_IGNORE);
        probe++;
    } while (!completed && probe < probe_count);
    if (!completed) {
        return UCC_INPROGRESS;
    }
    return UCC_OK;
}

static ucc_status_t oob_allgather_free(void *req)
{
    free(req);
    return UCC_OK;
}

static ucc_status_t oob_allgather(void *sbuf, void *rbuf, size_t msglen,
                                  void *oob_coll_ctx, void **req)
{
    oob_allgather_req_t *oob_req = malloc(sizeof(*oob_req));
    oob_req->sbuf                = sbuf;
    oob_req->rbuf                = rbuf;
    oob_req->msglen              = msglen;
    oob_req->oob_coll_ctx        = oob_coll_ctx;
    oob_req->iter                = 0;
    *req                         = oob_req;
    return UCC_OK;
}

static int mca_coll_ucc_init_ctx() {
    mca_coll_ucc_component_t     *cm = &mca_coll_ucc_component;
    ompi_attribute_fn_ptr_union_t del_fn;
    ompi_attribute_fn_ptr_union_t copy_fn;
    ucc_lib_config_h              lib_config;
    ucc_context_config_h          ctx_config;

    ucc_lib_params_t lib_params = {
        .mask = UCC_LIB_PARAM_FIELD_THREAD_MODE,
        .thread_mode = UCC_THREAD_SINGLE //TODO
    };
    ucc_context_params_t ctx_params = {
        .mask     = UCC_CONTEXT_PARAM_FIELD_TYPE,
        .ctx_type = UCC_CONTEXT_EXCLUSIVE,
        /* .oob = { */
        /*     .allgather    = oob_allgather, */
        /*     .req_test     = oob_allgather_test, */
        /*     .req_free     = oob_allgather_free, */
        /*     .coll_context = (void*)MPI_COMM_WORLD, */
        /*     .rank         = ompi_comm_rank(&ompi_mpi_comm_world.comm), */
        /*     .size         = ompi_comm_size(&ompi_mpi_comm_world.comm) */
        /* }, */
    };

    if (UCC_OK != ucc_lib_config_read("OMPI", NULL, &lib_config)) {
        UCC_ERROR("UCC lib config read failed");
        return OMPI_ERROR;
    }
    if (UCC_OK != ucc_lib_config_modify(lib_config, "CLS", cm->cls)) {
        ucc_lib_config_release(lib_config);
        UCC_ERROR("failed to modify UCC lib config to set CLS");
        return OMPI_ERROR;
    }

    if (UCC_OK != ucc_init(&lib_params, lib_config, &cm->ucc_lib)) {
        UCC_ERROR("UCC lib init failed");
        ucc_lib_config_release(lib_config);
        cm->ucc_enable = 0;
        return OMPI_ERROR;
    }
    ucc_lib_config_release(lib_config);

    if (UCC_OK != ucc_context_config_read(cm->ucc_lib, NULL, &ctx_config)) {
        UCC_ERROR("UCC context config read failed");
        goto cleanup_lib;
    }
    if (UCC_OK != ucc_context_create(cm->ucc_lib, &ctx_params,
                                     ctx_config, &cm->ucc_context)) {
        UCC_ERROR("UCC context create failed");
        ucc_context_config_release(ctx_config);
        goto cleanup_lib;
    }
    ucc_context_config_release(ctx_config);

    copy_fn.attr_communicator_copy_fn  = (MPI_Comm_internal_copy_attr_function*) MPI_COMM_NULL_COPY_FN;
    del_fn.attr_communicator_delete_fn = ucc_comm_attr_del_fn;
    if (OMPI_SUCCESS != ompi_attr_create_keyval(COMM_ATTR, copy_fn, del_fn,
                                                &ucc_comm_attr_keyval, NULL ,0, NULL)) {
        UCC_ERROR("UCC comm keyval create failed");
        goto cleanup_ctx;
    }
    opal_progress_register(mca_coll_ucc_progress);
    /* cm->ucc_ctx_attr.field_mask = UCC_CTX_ATTR_FIELD_SUPPORTED_COLLS; */
    /* if (UCC_OK!= ucc_ctx_query(cm->ucc_context, &cm->ucc_ctx_attr)) { */
    /*     UCC_ERROR("UCC failed to query context attributes"); */
    /*     goto cleanup_ctx; */
    /* } */
    UCC_VERBOSE(1, "initialized ucc context");
    cm->libucc_initialized = true;
    return OMPI_SUCCESS;
cleanup_ctx:
    ucc_context_destroy(cm->ucc_context);

cleanup_lib:
    ucc_finalize(cm->ucc_lib);
    cm->ucc_enable         = 0;
    cm->libucc_initialized = false;
    return OMPI_ERROR;
}
/*
 * Initialize module on the communicator
 */
static int mca_coll_ucc_module_enable(mca_coll_base_module_t *module,
                                      struct ompi_communicator_t *comm)
{
    mca_coll_ucc_component_t *cm         = &mca_coll_ucc_component;
    mca_coll_ucc_module_t    *ucc_module = (mca_coll_ucc_module_t *)module;
    int rc;
    UCC_VERBOSE(2,"creating ucc_team for comm %p, comm_id %d, comm_size %d",
                 (void*)comm,comm->c_contextid,ompi_comm_size(comm));
    ucc_team_params_t team_params = {
        .mask               = UCC_TEAM_PARAM_FIELD_EP |
                              UCC_TEAM_PARAM_FIELD_EP_RANGE |
                              UCC_TEAM_PARAM_FIELD_OOB,
        .oob   = {
            .allgather      = oob_allgather,
            .req_test       = oob_allgather_test,
            .req_free       = oob_allgather_free,
            .coll_info      = (void*)comm,
            .participants   = ompi_comm_size(comm)
        },
        .ep = ompi_comm_rank(comm),
    };
    if (UCC_OK != ucc_team_create_post(&cm->ucc_context, 1,
                                       &team_params, &ucc_module->ucc_team)) {
        UCC_ERROR("ucc_team_create_post failed");
        OBJ_RELEASE(ucc_module);
        goto err;
    }
    while (UCC_INPROGRESS == ucc_team_create_test(ucc_module->ucc_team)) {
        opal_progress();
    }

    if (OMPI_SUCCESS != mca_coll_ucc_save_coll_handlers(ucc_module)){
        UCC_ERROR("mca_coll_ucc_save_coll_handlers failed");
        goto err;
    }

    rc = ompi_attr_set_c(COMM_ATTR, comm, &comm->c_keyhash,
                         ucc_comm_attr_keyval, (void *)module, false);
    if (OMPI_SUCCESS != rc) {
        UCC_ERROR("ucc ompi_attr_set_c failed");
        goto err;
    }
    return OMPI_SUCCESS;

err:
    cm->ucc_enable = 0;
    opal_progress_unregister(mca_coll_ucc_progress);
    return OMPI_ERROR;
}


/*
 * Invoked when there's a new communicator that has been created.
 * Look at the communicator and decide which set of functions and
 * priority we want to return.
 */
mca_coll_base_module_t *
mca_coll_ucc_comm_query(struct ompi_communicator_t *comm, int *priority)
{
    mca_coll_ucc_component_t *cm = &mca_coll_ucc_component;
    mca_coll_ucc_module_t    *ucc_module;
    *priority = 0;

    if (!cm->ucc_enable){
        return NULL;
    }

    if (OMPI_COMM_IS_INTER(comm) || ompi_comm_size(comm) < cm->ucc_np
        || ompi_comm_size(comm) < 2){
        return NULL;
    }

    if (!cm->libucc_initialized) {
        if (OMPI_SUCCESS != mca_coll_ucc_init_ctx()) {
            cm->ucc_enable = 0;
            return NULL;
        }
    }

    ucc_module = OBJ_NEW(mca_coll_ucc_module_t);
    if (!ucc_module) {
        cm->ucc_enable = 0;
        return NULL;
    }
    ucc_module->comm                     = comm;
    ucc_module->super.coll_module_enable = mca_coll_ucc_module_enable;
    ucc_module->super.coll_barrier       = mca_coll_ucc_barrier;
        /* cm->ucc_ctx_attr.supported_colls & UCC_COLL_CAP_BARRIER ? */
        /* mca_coll_ucc_barrier : NULL; */
    /* ucc_module->super.coll_allreduce     = */
    /*     cm->ucc_ctx_attr.supported_colls & UCC_COLL_CAP_ALLREDUCE ? */
    /*     mca_coll_ucc_allreduce : NULL; */
    /* ucc_module->super.coll_reduce     = */
    /*     cm->ucc_ctx_attr.supported_colls & UCC_COLL_CAP_REDUCE ? */
    /*     mca_coll_ucc_reduce : NULL; */
    /* ucc_module->super.coll_bcast         = */
    /*     cm->ucc_ctx_attr.supported_colls & UCC_COLL_CAP_BCAST ? */
    /*     mca_coll_ucc_bcast : NULL; */
    /* ucc_module->super.coll_alltoall      = */
    /*     cm->ucc_ctx_attr.supported_colls & UCC_COLL_CAP_ALLTOALL ? */
    /*     mca_coll_ucc_alltoall : NULL; */
    *priority                            = cm->ucc_priority;
    return &ucc_module->super;
}


OBJ_CLASS_INSTANCE(mca_coll_ucc_module_t,
        mca_coll_base_module_t,
        mca_coll_ucc_module_construct,
        mca_coll_ucc_module_destruct);
