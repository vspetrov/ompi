/**
 * Copyright (c) 2020 Mellanox Technologies. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#include "coll_uch.h"
#include "coll_uch_dtypes.h"

int uch_comm_attr_keyval;
/*
 * Initial query function that is invoked during MPI_INIT, allowing
 * this module to indicate what level of thread support it provides.
 */
int mca_coll_uch_init_query(bool enable_progress_threads, bool enable_mpi_threads)
{
    return OMPI_SUCCESS;
}

static void mca_coll_uch_module_clear(mca_coll_uch_module_t *uch_module)
{
    uch_module->uch_comm         = NULL;
    uch_module->previous_allreduce  = NULL;
    uch_module->previous_barrier  = NULL;
}

static void mca_coll_uch_module_construct(mca_coll_uch_module_t *uch_module)
{
    mca_coll_uch_module_clear(uch_module);
}

#define OBJ_RELEASE_IF_NOT_NULL( obj ) if( NULL != (obj) ) OBJ_RELEASE( obj );

static void mca_coll_uch_module_destruct(mca_coll_uch_module_t *uch_module)
{
    int context_destroyed;
    if (uch_module->comm == &ompi_mpi_comm_world.comm){
        if (OMPI_SUCCESS != ompi_attr_free_keyval(COMM_ATTR, &uch_comm_attr_keyval, 0)) {
            UCH_VERBOSE(1,"uch ompi_attr_free_keyval failed");
        }
    }

    /* If the uch_context is null then we are destroying the uch_module
       that didn't initialized fallback colls/modules.
       Then just clear and return. Otherwise release module pointers and
       destroy uch context*/

    if (uch_module->uch_comm != NULL){
        OBJ_RELEASE_IF_NOT_NULL(uch_module->previous_allreduce_module);
        OBJ_RELEASE_IF_NOT_NULL(uch_module->previous_barrier_module);
    }
    mca_coll_uch_module_clear(uch_module);
}

#define SAVE_PREV_COLL_API(__api) do {                                  \
        uch_module->previous_ ## __api            = comm->c_coll->coll_ ## __api; \
        uch_module->previous_ ## __api ## _module = comm->c_coll->coll_ ## __api ## _module; \
        if (!comm->c_coll->coll_ ## __api || !comm->c_coll->coll_ ## __api ## _module) { \
            return OMPI_ERROR;                                          \
        }                                                               \
        OBJ_RETAIN(uch_module->previous_ ## __api ## _module);          \
    } while(0)

static int mca_coll_uch_save_coll_handlers(mca_coll_uch_module_t *uch_module)
{
    ompi_communicator_t *comm;
    comm = uch_module->comm;
    SAVE_PREV_COLL_API(allreduce);
    SAVE_PREV_COLL_API(barrier);
    return OMPI_SUCCESS;
}



/*
** Communicator free callback
*/
static int uch_comm_attr_del_fn(MPI_Comm comm, int keyval, void *attr_val, void *extra)
{

    mca_coll_uch_module_t *uch_module;
    uch_module = (mca_coll_uch_module_t*) attr_val;
    uch_comm_free(uch_module->uch_comm);
    return OMPI_SUCCESS;
}


static int oob_allgather(void *sbuf, void *rbuf, size_t msglen,
                          int my_rank, int *ranks, int nranks,  void *oob_coll_ctx) {
    ompi_communicator_t *comm = (ompi_communicator_t *)oob_coll_ctx;
    if (!comm) comm = &ompi_mpi_comm_world.comm;
    if (ranks == NULL) {
        comm->c_coll->coll_allgather(sbuf, msglen, MPI_BYTE,
                                     rbuf, msglen, MPI_BYTE, comm,
                                     comm->c_coll->coll_allgather_module);
    } else {
        if (my_rank == ranks[0]) {
            int i;
            memcpy(rbuf, sbuf, msglen);
            for (i=1; i<nranks; i++) {
                MCA_PML_CALL(recv((void*)((char*)rbuf + msglen*i), msglen,
                                  MPI_BYTE, ranks[i],
                                  MCA_COLL_BASE_TAG_UCH,
                                  comm, MPI_STATUS_IGNORE));
            }
            for (i=1; i<nranks; i++) {
                MCA_PML_CALL(send(rbuf, msglen*nranks, MPI_BYTE, ranks[i],
                                  MCA_COLL_BASE_TAG_UCH,
                                  MCA_PML_BASE_SEND_STANDARD, comm));
            }
        } else {
            MCA_PML_CALL(send(sbuf, msglen, MPI_BYTE, ranks[0],
                              MCA_COLL_BASE_TAG_UCH,
                              MCA_PML_BASE_SEND_STANDARD, comm));
            MCA_PML_CALL(recv(rbuf, msglen*nranks, MPI_BYTE, ranks[0],
                              MCA_COLL_BASE_TAG_UCH,
                              comm, MPI_STATUS_IGNORE));
        }
    }
    return 0;
}

static int oob_allgather_ctx(void *sbuf, void *rbuf, size_t msglen, void* oob_coll_ctx) {
    ompi_communicator_t *comm = &ompi_mpi_comm_world.comm;

    comm->c_coll->coll_allgather(sbuf, msglen, MPI_BYTE,
                                 rbuf, msglen, MPI_BYTE, comm,
                                 comm->c_coll->coll_allgather_module);
    return 0;
}

/*
 * Initialize module on the communicator
 */
static int mca_coll_uch_module_enable(mca_coll_base_module_t *module,
                                      struct ompi_communicator_t *comm)
{
    int rc;
    mca_coll_uch_module_t *uch_module;
    ompi_attribute_fn_ptr_union_t del_fn;
    ompi_attribute_fn_ptr_union_t copy_fn;
    mca_coll_uch_component_t *cm =
        &mca_coll_uch_component;
    if (!cm->libuch_initialized)
    {
        /* opal_progress_register(uch_progress_fn); */

        UCH_VERBOSE(10,"Calling uch_init();");
        uch_config_t config = {
            .flags = 0,
            .world_size = ompi_comm_size(&ompi_mpi_comm_world.comm),
            .world_rank = ompi_comm_rank(&ompi_mpi_comm_world.comm),
            .allgather = oob_allgather_ctx,
            .oob_coll_ctx = NULL,
        };

        rc = uch_init_context(&config, &cm->uch_context);
        if (UCC_OK != rc){
            cm->uch_enable = 0;
            /* opal_progress_unregister(uch_progress_fn); */
            UCH_ERROR("Hcol library init failed");
            return OMPI_ERROR;
        }
        copy_fn.attr_communicator_copy_fn = (MPI_Comm_internal_copy_attr_function*) MPI_COMM_NULL_COPY_FN;
        del_fn.attr_communicator_delete_fn = uch_comm_attr_del_fn;
        rc = ompi_attr_create_keyval(COMM_ATTR, copy_fn, del_fn, &uch_comm_attr_keyval, NULL ,0, NULL);
        if (OMPI_SUCCESS != rc) {
            cm->uch_enable = 0;
            /* opal_progress_unregister(uch_progress_fn); */
            /* uch_finalize(); */
            UCH_ERROR("Hcol comm keyval create failed");
            return OMPI_ERROR;
        }
        cm->libuch_initialized = true;
    }

    UCH_VERBOSE(10,"Creating uch_context for comm %p, comm_id %d, comm_size %d",
                 (void*)comm,comm->c_contextid,ompi_comm_size(comm));

    uch_comm_config_t comm_config = {
        .allgather = oob_allgather,
        .oob_coll_ctx = (void*)comm,
        .uch_ctx = cm->uch_context,
        .is_world = (comm == &ompi_mpi_comm_world.comm ? 1 : 0),
        .world_rank = ompi_comm_rank(&ompi_mpi_comm_world.comm),
        .comm_size  = ompi_comm_size(comm),
        .comm_rank  = ompi_comm_rank(comm),
        .caps.tagged_colls = 0,
    };

    uch_module = (mca_coll_uch_module_t *)module;
    if (UCC_OK != uch_comm_create(&comm_config, &uch_module->uch_comm)) {
        UCH_VERBOSE(1,"uch_create_context returned NULL");
        OBJ_RELEASE(uch_module);
        if (!cm->libuch_initialized) {
            cm->uch_enable = 0;
            /* uch_finalize(); */
            /* opal_progress_unregister(uch_progress_fn); */
        }
        return OMPI_ERROR;
    }

    if (OMPI_SUCCESS != mca_coll_uch_save_coll_handlers((mca_coll_uch_module_t *)module)){
        UCH_ERROR("coll_uch: mca_coll_uch_save_coll_handlers failed");
        return OMPI_ERROR;
    }

    rc = ompi_attr_set_c(COMM_ATTR, comm, &comm->c_keyhash, uch_comm_attr_keyval, (void *)module, false);
    if (OMPI_SUCCESS != rc) {
        UCH_VERBOSE(1,"uch ompi_attr_set_c failed");
        return OMPI_ERROR;
    }

    return OMPI_SUCCESS;
}


/*
 * Invoked when there's a new communicator that has been created.
 * Look at the communicator and decide which set of functions and
 * priority we want to return.
 */
mca_coll_base_module_t *
mca_coll_uch_comm_query(struct ompi_communicator_t *comm, int *priority)
{
    int err;
    int rc;
    mca_coll_uch_module_t *uch_module;
    mca_coll_uch_component_t *cm =
        &mca_coll_uch_component;
    *priority = 0;

    if (!cm->uch_enable){
        return NULL;
    }

    if (OMPI_COMM_IS_INTER(comm) || ompi_comm_size(comm) < cm->uch_np
        || ompi_comm_size(comm) < 2){
        return NULL;
    }

    uch_module = OBJ_NEW(mca_coll_uch_module_t);
    if (!uch_module){
        if (!cm->libuch_initialized) {
            cm->uch_enable = 0;
            /* uch_finalize(); */
            /* opal_progress_unregister(uch_progress_fn); */
        }
        return NULL;
    }

    uch_module->comm = comm;
    uch_module->super.coll_module_enable = mca_coll_uch_module_enable;
    uch_module->super.coll_allreduce     = mca_coll_uch_allreduce;
    uch_module->super.coll_barrier       = mca_coll_uch_barrier;
    *priority = cm->uch_priority;
    return &uch_module->super;
}


OBJ_CLASS_INSTANCE(mca_coll_uch_module_t,
        mca_coll_base_module_t,
        mca_coll_uch_module_construct,
        mca_coll_uch_module_destruct);



