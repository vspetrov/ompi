/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020 Mellanox Technologies. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include <dlfcn.h>
#include <libgen.h>

#include "coll_ucc.h"
#include "opal/mca/installdirs/installdirs.h"
#include "coll_ucc_dtypes.h"

static int mca_coll_ucc_open(void);
static int mca_coll_ucc_close(void);
static int mca_coll_ucc_register(void);
int mca_coll_ucc_output = -1;
mca_coll_ucc_component_t mca_coll_ucc_component = {
    /* First, the mca_component_t struct containing meta information
       about the component  */
    {
        .collm_version = {
            MCA_COLL_BASE_VERSION_2_0_0,

            /* Component name and version */
            .mca_component_name = "ucc",
            MCA_BASE_MAKE_VERSION(component, OMPI_MAJOR_VERSION, OMPI_MINOR_VERSION,
                                  OMPI_RELEASE_VERSION),

            /* Component open and close functions */
            .mca_open_component            = mca_coll_ucc_open,
            .mca_close_component           = mca_coll_ucc_close,
            .mca_register_component_params = mca_coll_ucc_register,
            .mca_query_component           = NULL,
        },
        .collm_data = {
            /* The component is not checkpoint ready */
            MCA_BASE_METADATA_PARAM_NONE
        },

        /* Initialization / querying functions */
        .collm_init_query = mca_coll_ucc_init_query,
        .collm_comm_query = mca_coll_ucc_comm_query,
    },
    100,               /* ucc_priority */
    0,                 /* ucc_verbose  */
    1,                 /* ucc_enable   */
    2,                 /* ucc_np       */
    "basic",           /* cls          */
    UCC_VERSION_STRING /* ucc version  */
};

static int mca_coll_ucc_register(void)
{
    mca_coll_ucc_component_t *cs = &mca_coll_ucc_component;
    mca_base_component_t     *c  = &cs->super.collm_version;

    mca_base_component_var_register(c, "priority", "Priority of the UCC coll component",
                                    MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                    OPAL_INFO_LVL_9,
                                    MCA_BASE_VAR_SCOPE_READONLY, &cs->ucc_priority);

    mca_base_component_var_register(c, "verbose", "Verbose level of the UCC coll component",
                                    MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                    OPAL_INFO_LVL_9,
                                    MCA_BASE_VAR_SCOPE_READONLY, &cs->ucc_verbose);

    mca_base_component_var_register(c, "enable", "[0|1] Enable/Disable the UCC coll component",
                                    MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                    OPAL_INFO_LVL_9,
                                    MCA_BASE_VAR_SCOPE_READONLY, &cs->ucc_enable);

    mca_base_component_var_register(c, "np", "Minimal communicator size for the UCC coll component",
                                    MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                    OPAL_INFO_LVL_9,
                                    MCA_BASE_VAR_SCOPE_READONLY, &cs->ucc_np);

    mca_base_component_var_register(c, MCA_COMPILETIME_VER,
                                    "Version of the libucc library with which Open MPI was compiled",
                                    MCA_BASE_VAR_TYPE_VERSION_STRING, NULL, 0, 0,
                                    OPAL_INFO_LVL_3, MCA_BASE_VAR_SCOPE_READONLY,
                                    &cs->compiletime_version);

    /* cs->runtime_version = ucc_get_version();     */
    mca_base_component_var_register(c, MCA_RUNTIME_VER,
                                    "Version of the libucc library with which Open MPI is running",
                                    MCA_BASE_VAR_TYPE_VERSION_STRING, NULL, 0, 0,
                                    OPAL_INFO_LVL_3, MCA_BASE_VAR_SCOPE_READONLY,
                                    &cs->runtime_version);

    mca_base_component_var_register(c, "cls",
                                    "Comma separated list of UCC CLS to be used for team creation",
                                    MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                    OPAL_INFO_LVL_6, MCA_BASE_VAR_SCOPE_READONLY, &cs->cls);
    return OMPI_SUCCESS;
}

static int mca_coll_ucc_open(void)
{
    mca_coll_ucc_component_t *cm = &mca_coll_ucc_component;
    mca_coll_ucc_output          = opal_output_open(NULL);
    cm->libucc_initialized       = false;
    opal_output_set_verbosity(mca_coll_ucc_output, cm->ucc_verbose);
    return OMPI_SUCCESS;
}

static int mca_coll_ucc_close(void)
{
    return OMPI_SUCCESS;
}
