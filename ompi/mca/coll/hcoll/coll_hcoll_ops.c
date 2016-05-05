/**
  Copyright (c) 2011      Mellanox Technologies. All rights reserved.
  Copyright (c) 2015      Research Organization for Information Science
                          and Technology (RIST). All rights reserved.
  $COPYRIGHT$

  Additional copyrights may follow

  $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"
#include "coll_hcoll.h"
#include "hcoll/api/hcoll_constants.h"
#include "coll_hcoll_dtypes.h"
#include "hcoll/api/hcoll_dte.h"
int mca_coll_hcoll_barrier(struct ompi_communicator_t *comm,
                         mca_coll_base_module_t *module){
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL BARRIER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rc = hcoll_collectives.coll_barrier(hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK BARRIER");
        rc = hcoll_module->previous_barrier(comm,hcoll_module->previous_barrier_module);
    }
    return rc;
}

int mca_coll_hcoll_bcast(void *buff, int count,
                        struct ompi_datatype_t *datatype, int root,
                        struct ompi_communicator_t *comm,
                        mca_coll_base_module_t *module)
{
    dte_data_representation_t dtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL BCAST");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    dtype = ompi_dtype_2_dte_dtype(datatype, TRY_FIND_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(dtype) || HCOL_DTE_IS_COMPLEX(dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: %s; calling fallback bcast;",datatype->super.name);
        rc = hcoll_module->previous_bcast(buff,count,datatype,root,
                                         comm,hcoll_module->previous_bcast_module);
        return rc;
    }
    rc = hcoll_collectives.coll_bcast(buff,count,dtype,root,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK BCAST");
        rc = hcoll_module->previous_bcast(buff,count,datatype,root,
                                         comm,hcoll_module->previous_bcast_module);
    }
    return rc;
}

int mca_coll_hcoll_allgather(const void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, int rcount,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLGATHER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, TRY_FIND_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, TRY_FIND_DERIVED);
    if (sbuf == MPI_IN_PLACE) {
        stype = rtype;
    }
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback allgather;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_allgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             hcoll_module->previous_allgather_module);
        return rc;
    }
    rc = hcoll_collectives.coll_allgather((void *)sbuf,scount,stype,rbuf,rcount,rtype,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLGATHER");
        rc = hcoll_module->previous_allgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             hcoll_module->previous_allgather_module);
    }
    return rc;
}

int mca_coll_hcoll_allgatherv(const void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, const int *rcount,
                            const int *displs,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLGATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback allgatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_allgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             hcoll_module->previous_allgatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_allgatherv((void *)sbuf,scount,stype,rbuf,rcount,displs,rtype,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLGATHERV");
        rc = hcoll_module->previous_allgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             hcoll_module->previous_allgatherv_module);
    }
    return rc;
}

int mca_coll_hcoll_gather(const void *sbuf, int scount,
                          struct ompi_datatype_t *sdtype,
                          void *rbuf, int rcount,
                          struct ompi_datatype_t *rdtype,
                          int root,
                          struct ompi_communicator_t *comm,
                          mca_coll_base_module_t *module){
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL GATHER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback gather;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_gather(sbuf,scount,sdtype,
                                           rbuf,rcount,rdtype,root,
                                           comm,
                                           hcoll_module->previous_allgather_module);
        return rc;
    }
    rc = hcoll_collectives.coll_gather((void *)sbuf,scount,stype,rbuf,rcount,rtype,root,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK GATHER");
        rc = hcoll_module->previous_gather((void *)sbuf,scount,sdtype,
                                              rbuf,rcount,rdtype,root,
                                              comm,
                                              hcoll_module->previous_allgather_module);
    }
    return rc;

}

int mca_coll_hcoll_allreduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLREDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback allreduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_allreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, hcoll_module->previous_allreduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback allreduce;",
                     op->o_name);
        rc = hcoll_module->previous_allreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, hcoll_module->previous_allreduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_allreduce((void *)sbuf,rbuf,count,Dtype,Op,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLREDUCE");
        rc = hcoll_module->previous_allreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, hcoll_module->previous_allreduce_module);
    }
    return rc;
}

int mca_coll_hcoll_reduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            int root,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL REDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback reduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_reduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, hcoll_module->previous_reduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback reduce;",
                     op->o_name);
        rc = hcoll_module->previous_reduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, hcoll_module->previous_reduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_reduce((void *)sbuf,rbuf,count,Dtype,Op,root,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK REDUCE");
        rc = hcoll_module->previous_reduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, hcoll_module->previous_reduce_module);
    }
    return rc;
}

int mca_coll_hcoll_alltoall(const void *sbuf, int scount,
                           struct ompi_datatype_t *sdtype,
                           void* rbuf, int rcount,
                           struct ompi_datatype_t *rdtype,
                           struct ompi_communicator_t *comm,
                           mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLTOALL");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback alltoall;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_alltoall(sbuf,scount,sdtype,
                                            rbuf,rcount,rdtype,
                                            comm,
                                            hcoll_module->previous_alltoall_module);
        return rc;
    }
    rc = hcoll_collectives.coll_alltoall((void *)sbuf,scount,stype,rbuf,rcount,rtype,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLTOALL");
        rc = hcoll_module->previous_alltoall(sbuf,scount,sdtype,
                                            rbuf,rcount,rdtype,
                                            comm,
                                            hcoll_module->previous_alltoall_module);
    }
    return rc;
}

int mca_coll_hcoll_alltoallv(const void *sbuf, const int *scounts, const int *sdisps,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, const int *rcounts, const int *rdisps,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLTOALLV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback alltoallv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_alltoallv(sbuf, scounts, sdisps, sdtype,
                                            rbuf, rcounts, rdisps, rdtype,
                                            comm, hcoll_module->previous_alltoallv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_alltoallv((void *)sbuf, (int *)scounts, (int *)sdisps, stype,
                                            rbuf, (int *)rcounts, (int *)rdisps, rtype,
                                                hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLTOALLV");
        rc = hcoll_module->previous_alltoallv(sbuf, scounts, sdisps, sdtype,
                                            rbuf, rcounts, rdisps, rdtype,
                                            comm, hcoll_module->previous_alltoallv_module);
    }
    return rc;
}

int mca_coll_hcoll_gatherv(const void* sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void* rbuf, const int *rcounts, const int *displs,
                            struct ompi_datatype_t *rdtype,
                            int root,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL GATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback gatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_gatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, hcoll_module->previous_gatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_gatherv((void *)sbuf, scount, stype, rbuf, (int *)rcounts, (int *)displs, rtype, root, hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK GATHERV");
        rc = hcoll_module->previous_gatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, hcoll_module->previous_igatherv_module);
    }
    return rc;

}

int mca_coll_hcoll_ibarrier(struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING BARRIER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    rc = hcoll_collectives.coll_ibarrier(hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING BARRIER");
        rc = hcoll_module->previous_ibarrier(comm, request, hcoll_module->previous_ibarrier_module);
    }
    return rc;
}

int mca_coll_hcoll_ibcast(void *buff, int count,
                        struct ompi_datatype_t *datatype, int root,
                        struct ompi_communicator_t *comm,
                        ompi_request_t ** request,
                        mca_coll_base_module_t *module)
{
    dte_data_representation_t dtype;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING BCAST");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    dtype = ompi_dtype_2_dte_dtype(datatype, TRY_FIND_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(dtype) || HCOL_DTE_IS_COMPLEX(dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: %s; calling fallback non-blocking bcast;",datatype->super.name);
        rc = hcoll_module->previous_ibcast(buff,count,datatype,root,
                                         comm, request, hcoll_module->previous_ibcast_module);
        return rc;
    }
    rc = hcoll_collectives.coll_ibcast(buff, count, dtype, root, rt_handle, hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING BCAST");
        rc = hcoll_module->previous_ibcast(buff,count,datatype,root,
                                         comm, request, hcoll_module->previous_ibcast_module);
    }
    return rc;
}

int mca_coll_hcoll_iallgather(const void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, int rcount,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING ALLGATHER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback non-blocking allgather;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_iallgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgather_module);
        return rc;
    }
    rc = hcoll_collectives.coll_iallgather((void *)sbuf, scount, stype, rbuf, rcount, rtype, hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING ALLGATHER");
        rc = hcoll_module->previous_iallgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgather_module);
    }
    return rc;
}
#if HCOLL_API >= HCOLL_VERSION(3,5)
int mca_coll_hcoll_iallgatherv(const void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, const int *rcount,
                            const int *displs,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING ALLGATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    void **rt_handle = (void **) request;
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback non-blocking allgatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_iallgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_iallgatherv((void *)sbuf,scount,stype,rbuf,rcount,displs,rtype,
            hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
       HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING ALLGATHER");
        rc = hcoll_module->previous_iallgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgatherv_module);
    }
    return rc;
}
#endif
int mca_coll_hcoll_iallreduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING ALLREDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback non-blocking allreduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_iallreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, request, hcoll_module->previous_iallreduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback non-blocking allreduce;",
                     op->o_name);
        rc = hcoll_module->previous_iallreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, request, hcoll_module->previous_iallreduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_iallreduce((void *)sbuf, rbuf, count, Dtype, Op, hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING ALLREDUCE");
        rc = hcoll_module->previous_iallreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, request, hcoll_module->previous_iallreduce_module);
    }
    return rc;
}
#if HCOLL_API >= HCOLL_VERSION(3,5)
int mca_coll_hcoll_ireduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            int root,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING REDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    void **rt_handle = (void**) request;
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback non-blocking reduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_ireduce(sbuf,rbuf,count,dtype,op,
                                             root,
                                             comm, request, 
                                             hcoll_module->previous_ireduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback non-blocking reduce;",
                     op->o_name);
        rc = hcoll_module->previous_ireduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, request,
                                             hcoll_module->previous_ireduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_ireduce((void *)sbuf,rbuf,count,Dtype,Op,root,hcoll_module->hcoll_context,rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING REDUCE");
        rc = hcoll_module->previous_ireduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, 
                                             request,
                                             hcoll_module->previous_ireduce_module);
    }
    return rc;
}
#endif
int mca_coll_hcoll_igatherv(const void* sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void* rbuf, const int *rcounts, const int *displs,
                            struct ompi_datatype_t *rdtype,
                            int root,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL IGATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback igatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_igatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, request,
                                           hcoll_module->previous_igatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_igatherv((void *)sbuf, scount, stype, rbuf, (int *)rcounts, (int *)displs, rtype, root, hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK IGATHERV");
        rc = hcoll_module->previous_igatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, request,
                                           hcoll_module->previous_igatherv_module);
    }
    return rc;

}

#if HCOLL_API >= HCOLL_VERSION(3,5)
static
int32_t hcoll_dtype_create_vector( int count, int bLength, int stride,
                                   ompi_datatype_t* oldType, dte_data_representation_t** newType )
{
    dte_data_representation_t parent_dte, *new_dte;
    parent_dte = ompi_dtype_2_dte_dtype(oldType, TRY_FIND_DERIVED);
    if (HCOL_DTE_IS_ZERO(parent_dte) || HCOL_DTE_IS_COMPLEX(parent_dte)){
        *newType = &DTE_ZERO;
        return OMPI_SUCCESS;
    }
    //TODO check return
    hcoll_dte_create_vector(count, bLength, stride, parent_dte, &new_dte);
    *newType = new_dte;
    return OMPI_SUCCESS;
}

static
int32_t hcoll_dtype_create_struct(int count, const int* pBlockLength, const OPAL_PTRDIFF_TYPE* pDisp,
                                  ompi_datatype_t** pTypes, dte_data_representation_t** newType )
{
    int i;
    dte_data_representation_t ptype, *new_dte;
    int is_const_length = 1;
    int is_const_stride = 1;
    int is_const_type   = 1;
    OPAL_PTRDIFF_TYPE stride = pDisp[1] - pDisp[0];
    for (i=0; i<count; i++) {
        ptype = ompi_dtype_2_dte_dtype(pTypes[i], TRY_FIND_DERIVED);
        if (HCOL_DTE_IS_ZERO(ptype) || HCOL_DTE_IS_COMPLEX(ptype)){
            /* we found not a simple dtype as an input for the new struct
               dtype, we will not support this for now */
            *newType = &DTE_ZERO;
            return OMPI_SUCCESS;
        }
        if (pBlockLength[i] != pBlockLength[0])
            is_const_length = 0;
        if (pTypes[i] != pTypes[0])
            is_const_type = 0;
        if (i > 1 && (pDisp[i]-pDisp[i-1] != stride))
            is_const_stride = 0;
    }
    if (is_const_length && is_const_stride && is_const_type && stride < INT_MAX)
        return hcoll_dtype_create_vector(count,pBlockLength[0],(int)stride,pTypes[0],newType);

    dte_data_representation_t *dte_types = (dte_data_representation_t *)
        malloc(count*sizeof(*dte_types));
    for (i=0; i<count; i++)
        dte_types[i] = ompi_dtype_2_dte_dtype(pTypes[i], TRY_FIND_DERIVED);

    hcoll_dte_create_struct(count, (int*)pBlockLength, (ptrdiff_t*)pDisp, dte_types, &new_dte);
    *newType = new_dte;
    free(dte_types);
    return OMPI_SUCCESS;
}

int hcoll_map_derived_type(ompi_datatype_t *dtype, dte_data_representation_t **new_dte)
{
    int rc;
    int num_integers, num_addresses, num_datatypes, combiner;
    const int max_stack_arr = 5;
    int array_of_integers[max_stack_arr];
    MPI_Aint array_of_addresses[max_stack_arr];
    MPI_Datatype array_of_datatypes[max_stack_arr];
    int *aoi = array_of_integers;
    MPI_Aint *aoa = array_of_addresses;
    MPI_Datatype *aod = array_of_datatypes;
    if (NULL == dtype->args) {
        /* predefined type, shouldn't call this */
        return OMPI_SUCCESS;
    }
    rc = ompi_datatype_get_args( dtype, 0, &num_integers, NULL, &num_addresses, NULL,
                                 &num_datatypes, NULL, &combiner );
    if (MPI_SUCCESS != rc) {
        HCOL_VERBOSE(1, "Failed to query dtype args size\n");
        return rc;
    }
    if (num_integers > max_stack_arr)
        aoi = (int*)malloc(num_integers*sizeof(int));
    if (num_addresses > max_stack_arr)
        aoa = (MPI_Aint*)malloc(num_integers*sizeof(MPI_Aint));
    if (num_datatypes > max_stack_arr)
        aod = (MPI_Datatype*)malloc(num_integers*sizeof(MPI_Datatype));

    rc = ompi_datatype_get_args( dtype, 1, &num_integers, aoi,
                                 &num_addresses, aoa,
                                 &num_datatypes, aod, NULL );
    if (MPI_SUCCESS != rc) {
        HCOL_VERBOSE(1, "Failed to query dtype args\n");
        goto Cleanup;
    }

    switch (combiner) {
    case MPI_COMBINER_VECTOR:
        hcoll_dtype_create_vector( aoi[0], aoi[1],
                                   aoi[2], aod[0],
                                   new_dte );
        break;
    case MPI_COMBINER_STRUCT:
        hcoll_dtype_create_struct( aoi[0], &aoi[1],
                                   aoa, aod,
                                   new_dte );
        break;
    default:
        break;
    };

    rc = ompi_attr_set_c(TYPE_ATTR, (void*)dtype, &(dtype->d_keyhash), hcoll_type_attr_keyval, (void *)(*new_dte), false);
    if (OMPI_SUCCESS != rc) {
        HCOL_VERBOSE(1,"hcoll ompi_attr_set_c failed for derived dtype");
        goto Cleanup;
    }
Cleanup:

    if (aoi != array_of_integers)  free(aoi);
    if (aoa != array_of_addresses) free(aoa);
    if (aod != array_of_datatypes) free(aod);
    
    return rc;
}
#else
int hcoll_map_derived_type(ompi_datatype_t *dtype, dte_data_representation_t **new_dte)
{
    /*Do nothing - it's an old version of hcoll w/o dtypes support */
    return OMPI_SUCCESS;
}

#endif
