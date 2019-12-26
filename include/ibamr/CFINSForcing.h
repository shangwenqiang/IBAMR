// ---------------------------------------------------------------------
//
// Copyright (c) 2019 - 2019 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

#ifndef included_CFINSForcing
#define included_CFINSForcing
/////////////////////////////// INCLUDES /////////////////////////////////////
#include "IBAMR_config.h"

#include "ibamr/AdvDiffSemiImplicitHierarchyIntegrator.h"
#include "ibamr/CFGiesekusRelaxation.h"
#include "ibamr/CFOldroydBRelaxation.h"
#include "ibamr/CFRoliePolyRelaxation.h"
#include "ibamr/CFUpperConvectiveOperator.h"
#include "ibamr/INSStaggeredHierarchyIntegrator.h"
#include "ibamr/ibamr_enums.h"

#include "ibtk/CartGridFunction.h"
#include "ibtk/HierarchyGhostCellInterpolation.h"
#include "ibtk/IBTK_MPI.h"
#include "ibtk/ibtk_macros.h"
#include "ibtk/ibtk_utilities.h"
#include "ibtk/muParserCartGridFunction.h"
#include "ibtk/muParserRobinBcCoefs.h"

#include "Box.h"
#include "CartesianPatchGeometry.h"
#include "CellData.h"
#include "CellIndex.h"
#include "CellVariable.h"
#include "HierarchyDataOpsManager.h"
#include "Index.h"
#include "IntVector.h"
#include "Patch.h"
#include "PatchGeometry.h"
#include "PatchHierarchy.h"
#include "PatchLevel.h"
#include "SideData.h"
#include "Variable.h"
#include "VariableContext.h"
#include "VariableDatabase.h"
#include "VisItDataWriter.h"
#include "tbox/Database.h"
#include "tbox/Pointer.h"

IBTK_DISABLE_EXTRA_WARNINGS
#include <Eigen/Cholesky>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <unsupported/Eigen/MatrixFunctions>
IBTK_ENABLE_EXTRA_WARNINGS

#include <string>
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class CFINSForcing provides an interface for specifying a viscoelastic stress to be added to the
 * Navier-Stokes equations. The class uses the advection diffusion integrator to update the viscoelastic stress.
 *
 * One can choose from the pre-programmed models Oldroyd-B, Giesekus, or Rolie-Poly. Optionally, you can also register a
 * relaxation operator that computes the relaxation function. The fluid model is specified through the database
 * parameter "fluid_parameter". By specifying "USER_DEFINED", you can register your own relaxation function. This class
 * currently solves for the conformation tensor or the square root or logarithm of the conformation tensor. The current
 * assumption is that the stress is linearly related to the conformation tensor through the elastic modulus.
 */
class CFINSForcing : public IBTK::CartGridFunction
{
public:
    /*!
     * \brief This constructor creates Variable and VariableContext objects for storing the viscoleastic stresses at the
     * centers of the Cartesian grid. Sets up the advection diffusion solver to use the velocity function prescribed.
     */
    CFINSForcing(const std::string& object_name,
                 SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
                 SAMRAI::tbox::Pointer<IBTK::CartGridFunction> u_fcn,
                 SAMRAI::tbox::Pointer<SAMRAI::geom::CartesianGridGeometry<NDIM> > grid_geometry,
                 SAMRAI::tbox::Pointer<IBAMR::AdvDiffSemiImplicitHierarchyIntegrator> adv_diff_integrator,
                 SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > visit_data_writer);

    /*!
     * \brief This constructor creates Variable and VariableContext objects for
     * storing the stochastic stresses at the centers and nodes of the Cartesian
     * grid.
     */
    CFINSForcing(const std::string& object_name,
                 SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> app_initializer,
                 const SAMRAI::tbox::Pointer<IBAMR::INSHierarchyIntegrator> fluid_solver,
                 SAMRAI::tbox::Pointer<SAMRAI::geom::CartesianGridGeometry<NDIM> > grid_geometry,
                 SAMRAI::tbox::Pointer<IBAMR::AdvDiffSemiImplicitHierarchyIntegrator> adv_diff_integrator,
                 SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > visit_data_writer);

    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    CFINSForcing() = delete;

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    CFINSForcing(const CFINSForcing& from) = delete;

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     */
    CFINSForcing& operator=(const CFINSForcing& that) = delete;

    /*!
     * \brief Deallocates draw data and deletes boundary conditions.
     */
    ~CFINSForcing();

    /*!
     * \brief This function returns a pointer to the cell variable that stores the viscoelastic stress.
     */
    inline SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > getVariable()
    {
        return d_W_cc_var;
    }

    /*!
     * \brief This function returns the patch data index used to store the viscoelastic stress.
     */
    inline int getVariableIdx()
    {
        return d_W_cc_idx;
    }

    /*!
     * \brief This function register a relaxation function with the class.
     *
     * \note This function is automatically called if you specify the Oldroyd-B, Giesekus, or Rolie-Poly models.
     */
    void registerRelaxationOperator(SAMRAI::tbox::Pointer<IBAMR::CFRelaxationOperator> rhs);

    /*!
     * \name Methods to set patch data.
     */
    //\{

    /*!
     * \brief Indicates whether the concrete INSStaggeredStochasticForcing object is
     * time-dependent.
     */
    bool isTimeDependent() const override;

    /*!
     * \brief Evaluate the function on the patch interiors on the specified
     * levels of the patch hierarchy.
     */
    void setDataOnPatchHierarchy(const int data_idx,
                                 SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                                 SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                 const double data_time,
                                 const bool initial_time = false,
                                 const int coarsest_ln = -1,
                                 const int finest_ln = -1) override;

    /*!
     * \brief Evaluate the function on the patch interior.
     */
    void setDataOnPatch(const int data_idx,
                        SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                        SAMRAI::tbox::Pointer<SAMRAI::hier::Patch<NDIM> > patch,
                        const double data_time,
                        const bool initial_time = false,
                        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchLevel<NDIM> > patch_level =
                            SAMRAI::tbox::Pointer<SAMRAI::hier::PatchLevel<NDIM> >(NULL)) override;

    void setDataOnPatchLevel(const int data_idx,
                             SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                             SAMRAI::tbox::Pointer<SAMRAI::hier::PatchLevel<NDIM> > level,
                             const double data_time,
                             const bool initial_time) override;
    //\}

    void checkPositiveDefinite(const int data_idx,
                               const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                               const double data_time,
                               const bool initial_time);

    void applyGradientDetector(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                               int level_number,
                               double error_data_time,
                               int tag_index,
                               bool initial_time,
                               bool /*richardson_extrapolation_too*/);

    static void
    apply_gradient_detector_callback(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                                     int level_number,
                                     double error_data_time,
                                     int tag_index,
                                     bool initial_time,
                                     bool richardson_extrapolation_too,
                                     void* ctx);

    inline double getViscosity()
    {
        return d_eta;
    }

    inline double getRelaxationTime()
    {
        return d_lambda;
    }

private:
    void commonConstructor(const SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
                           SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > visit_data_writer,
                           SAMRAI::tbox::Pointer<SAMRAI::geom::CartesianGridGeometry<NDIM> > grid_geometry,
                           std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*> vel_bcs);

    void findDeterminant(const int data_idx,
                         const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                         const double data_time,
                         const bool initial_time);

    void squareMatrix(const int data_idx,
                      const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                      const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                      const double data_time,
                      const bool initial_time,
                      const int coarsest_ln,
                      const int finest_ln);

    void exponentiateMatrix(const int data_idx,
                            const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                            const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                            const double data_time,
                            const bool initial_time,
                            const int coarsest_ln,
                            const int finest_ln);

    void projectTensor(const int data_idx,
                       const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > var,
                       const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                       const double data_time,
                       const bool initial_time,
                       const int coarsest_ln,
                       const int finest_ln);

    // Scratch variables
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_W_cc_var;
    SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext> d_context;
    int d_W_cc_idx = IBTK::invalid_index, d_W_scratch_idx = IBTK::invalid_index;
    SAMRAI::tbox::Pointer<IBTK::muParserCartGridFunction> d_init_conds;

    // Draw Variables
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM, double> > d_conform_var_draw, d_stress_var_draw,
        d_divW_var_draw;
    int d_conform_idx_draw = IBTK::invalid_index, d_stress_idx_draw = IBTK::invalid_index,
        d_divW_idx_draw = IBTK::invalid_index;
    bool d_conform_draw = true, d_stress_draw = true, d_divW_draw = false;

    // Complex Fluid parameters
    double d_lambda = std::numeric_limits<double>::quiet_NaN(), d_eta = std::numeric_limits<double>::quiet_NaN();

    // Extra parameters
    std::string d_fluid_model = "OLDROYDB", d_interp_type = "LINEAR";
    bool d_project_conform = true;
    TensorEvolutionType d_evolve_type = STANDARD;
    AdvDiffSemiImplicitHierarchyIntegrator* const d_adv_diff_integrator;
    SAMRAI::tbox::Pointer<CFUpperConvectiveOperator> d_convec_oper;
    std::string d_convec_oper_type;
    std::vector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*> d_conc_bc_coefs;
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_hierarchy;

    // Logging parameters
    double d_max_det = std::numeric_limits<double>::quiet_NaN(), d_min_det = std::numeric_limits<double>::quiet_NaN();
    bool d_log_det = false, d_log_divW = false;
    bool d_positive_def = true, d_error_on_spd = false;
    double d_min_norm = std::numeric_limits<double>::quiet_NaN(), d_max_norm = std::numeric_limits<double>::quiet_NaN();

    // AMR tagging
    SAMRAI::tbox::Array<double> d_divW_rel_thresh, d_divW_abs_thresh;
    bool d_divW_rel_tag = false, d_divW_abs_tag = false;

    // Velocity information
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_u_fcn;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM, double> > d_u_var;
};
} // Namespace IBAMR
#endif
