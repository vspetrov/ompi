/*
 * $HEADERS$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "mpi.h"
#include "mpi/c/bindings.h"
#include "mca/coll/coll.h"
#include "errhandler/errhandler.h"
#include "communicator/communicator.h"
#include "op/op.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Scan = PMPI_Scan
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Scan";


int MPI_Scan(void *sendbuf, void *recvbuf, int count,
             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) 
{
    int err;

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (ompi_comm_invalid(comm)) {
	    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COMM, 
                                          FUNC_NAME);
	}

        /* No intercommunicators allowed! (MPI does not define
           MPI_SCAN on intercommunicators) */

        if (OMPI_COMM_IS_INTER(comm)) {
	    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COMM, FUNC_NAME);
        }

        /* Unrooted operation; checks for all ranks */
	
	if (MPI_DATATYPE_NULL == datatype) {
	    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_TYPE, FUNC_NAME);
	}
	
	if (MPI_OP_NULL == op) {
	    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_OP, FUNC_NAME);
	}

        if (ompi_op_is_intrinsic(op) && datatype->id < DT_MAX_PREDEFINED &&
            -1 == ompi_op_ddt_map[datatype->id]) {
          return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_OP, FUNC_NAME);
        }

	if (count < 0) {
	    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COUNT, FUNC_NAME);
	}
    }

    /* Call the coll component to actually perform the allgather */

    err = comm->c_coll.coll_scan(sendbuf, recvbuf, count,
                                 datatype, op, comm);
    OMPI_ERRHANDLER_RETURN(err, comm, err, FUNC_NAME);
}
