#ifndef COLL_HCOLL_DTYPES_H
#define COLL_HCOLL_DTYPES_H

/*Here comes the translation between ompi_datatype_t and dte_data_representation
  This is not complete and takes into account the basic datatypes only
  It is used to extract allreduce bcol functions where the arrhythmetics has to be done*/

#include "ompi/datatype/ompi_datatype.h"
#include "ompi/mca/op/op.h"
#include "hcoll/api/hcoll_dte.h"
extern int hcoll_type_attr_keyval;

/*to keep this at hand: Ids of the basic opal_datatypes:
#define OPAL_DATATYPE_INT1           4
#define OPAL_DATATYPE_INT2           5
#define OPAL_DATATYPE_INT4           6
#define OPAL_DATATYPE_INT8           7
#define OPAL_DATATYPE_INT16          8
#define OPAL_DATATYPE_UINT1          9
#define OPAL_DATATYPE_UINT2          10
#define OPAL_DATATYPE_UINT4          11
#define OPAL_DATATYPE_UINT8          12
#define OPAL_DATATYPE_UINT16         13
#define OPAL_DATATYPE_FLOAT2         14
#define OPAL_DATATYPE_FLOAT4         15
#define OPAL_DATATYPE_FLOAT8         16
#define OPAL_DATATYPE_FLOAT12        17
#define OPAL_DATATYPE_FLOAT16        18
#define OPAL_DATATYPE_FLOAT_COMPLEX  19
#define OPAL_DATATYPE_DOUBLE_COMPLEX 20

total 15 types
*/



static dte_data_representation_t* ompi_datatype_2_dte_data_rep[OPAL_DATATYPE_MAX_PREDEFINED] = {
    &DTE_ZERO,                  /*OPAL_DATATYPE_LOOP           0 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_END_LOOP       1 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_LB             2 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_UB             3 */
    &DTE_BYTE,                  /*OPAL_DATATYPE_INT1           4 */
    &DTE_INT16,                 /*OPAL_DATATYPE_INT2           5 */
    &DTE_INT32,                 /*OPAL_DATATYPE_INT4           6 */
    &DTE_INT64,                 /*OPAL_DATATYPE_INT8           7 */
    &DTE_INT128,                /*OPAL_DATATYPE_INT16          8 */
    &DTE_UBYTE,                 /*OPAL_DATATYPE_UINT1          9 */
    &DTE_UINT16,                /*OPAL_DATATYPE_UINT2          10 */
    &DTE_UINT32,                /*OPAL_DATATYPE_UINT4          11 */
    &DTE_UINT64,                /*OPAL_DATATYPE_UINT8          12 */
    &DTE_UINT128,               /*OPAL_DATATYPE_UINT16         13 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_FLOAT2         14 */
    &DTE_FLOAT32,               /*OPAL_DATATYPE_FLOAT4         15 */
    &DTE_FLOAT64,               /*OPAL_DATATYPE_FLOAT8         16 */
    &DTE_FLOAT96,               /*OPAL_DATATYPE_FLOAT12        17 */
    &DTE_FLOAT128,              /*OPAL_DATATYPE_FLOAT16        18 */
#if defined(DTE_FLOAT32_COMPLEX) && defined(DTE_FLOAT64_COMPLEX)
    &DTE_FLOAT32_COMPLEX,       /*OPAL_DATATYPE_COMPLEX8       19 */
    &DTE_FLOAT64_COMPLEX,       /*OPAL_DATATYPE_COMPLEX16      20 */
#else
    &DTE_ZERO,                  /*OPAL_DATATYPE_COMPLEX8       19 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_COMPLEX16      20 */
#endif
    &DTE_ZERO,                  /*OPAL_DATATYPE_COMPLEX32      21 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_BOOL           22 */
    &DTE_ZERO,                  /*OPAL_DATATYPE_WCHAR          23 */
    &DTE_ZERO                   /*OPAL_DATATYPE_UNAVAILABLE    24 */
};

int hcoll_map_derived_type(ompi_datatype_t *dtype,
                           dte_data_representation_t **dte);

static dte_data_representation_t find_derived_mapping(ompi_datatype_t *dtype){
    dte_data_representation_t *dte = &DTE_ZERO;
    if (mca_coll_hcoll_component.derived_types_support_enabled) {
        if (!dtype->d_keyhash || OPAL_SUCCESS != opal_hash_table_get_value_uint32(dtype->d_keyhash,
                                                                                  hcoll_type_attr_keyval, (void**)&dte)) {
            hcoll_map_derived_type(dtype, &dte);
        }
    }
    return *dte;
}

enum {
    TRY_FIND_DERIVED,
    NO_DERIVED
};

static dte_data_representation_t ompi_dtype_2_dte_dtype( ompi_datatype_t *dtype,
                                                        const int mode)
{
    int ompi_type_id = dtype->id;
    int opal_type_id = dtype->super.id;
    dte_data_representation_t *dte_data_rep = NULL;
    if (!(dtype->super.flags & OPAL_DATATYPE_FLAG_NO_GAPS)) {
        ompi_type_id = -1;
    }
    if (OPAL_UNLIKELY( ompi_type_id < 0 ||
                       ompi_type_id >= OPAL_DATATYPE_MAX_PREDEFINED)){
        if (TRY_FIND_DERIVED == mode)
            return find_derived_mapping(dtype);
        return DTE_ZERO;
    }
    return *ompi_datatype_2_dte_data_rep[opal_type_id];
}

static hcoll_dte_op_t* ompi_op_2_hcoll_op[OMPI_OP_BASE_FORTRAN_OP_MAX + 1] = {
   &hcoll_dte_op_null,          /* OMPI_OP_BASE_FORTRAN_NULL = 0 */
   &hcoll_dte_op_max,           /* OMPI_OP_BASE_FORTRAN_MAX */
   &hcoll_dte_op_min,           /* OMPI_OP_BASE_FORTRAN_MIN */
   &hcoll_dte_op_sum,           /* OMPI_OP_BASE_FORTRAN_SUM */
   &hcoll_dte_op_prod,          /* OMPI_OP_BASE_FORTRAN_PROD */
   &hcoll_dte_op_land,          /* OMPI_OP_BASE_FORTRAN_LAND */
   &hcoll_dte_op_band,          /* OMPI_OP_BASE_FORTRAN_BAND */
   &hcoll_dte_op_lor,           /* OMPI_OP_BASE_FORTRAN_LOR */
   &hcoll_dte_op_bor,           /* OMPI_OP_BASE_FORTRAN_BOR */
   &hcoll_dte_op_lxor,          /* OMPI_OP_BASE_FORTRAN_LXOR */
   &hcoll_dte_op_bxor,          /* OMPI_OP_BASE_FORTRAN_BXOR */
   &hcoll_dte_op_null,          /* OMPI_OP_BASE_FORTRAN_MAXLOC */
   &hcoll_dte_op_null,          /* OMPI_OP_BASE_FORTRAN_MINLOC */
   &hcoll_dte_op_null,          /* OMPI_OP_BASE_FORTRAN_REPLACE */
   &hcoll_dte_op_null,          /* OMPI_OP_BASE_FORTRAN_NO_OP */
   &hcoll_dte_op_null           /* OMPI_OP_BASE_FORTRAN_OP_MAX */
};
static hcoll_dte_op_t* ompi_op_2_hcolrte_op(ompi_op_t *op) {
    if (op->o_f_to_c_index > OMPI_OP_BASE_FORTRAN_OP_MAX) {
        return ompi_op_2_hcoll_op[0]; /* return null */
    }
    return ompi_op_2_hcoll_op[op->o_f_to_c_index];
}




#if HCOLL_API >= HCOLL_VERSION(3,5)
static int hcoll_type_attr_del_fn(MPI_Datatype type, int keyval, void *attr_val, void *extra) {
    int ret = OMPI_SUCCESS;
    dte_data_representation_t *dte =
        (dte_data_representation_t*) attr_val;

    assert(dte);
    if (HCOLL_SUCCESS != (ret = hcoll_dte_destroy(dte))) {
        HCOL_ERROR("failed to delete type attr: hcoll_dte_destroy returned %d",ret);
        return OMPI_ERROR;
    }

    return OMPI_SUCCESS;
}
#else
static int hcoll_type_attr_del_fn(MPI_Datatype type, int keyval, void *attr_val, void *extra) {
    /*Do nothing - it's an old version of hcoll w/o dtypes support */
    return OMPI_SUCCESS;
}
#endif
#endif /* COLL_HCOLL_DTYPES_H */
