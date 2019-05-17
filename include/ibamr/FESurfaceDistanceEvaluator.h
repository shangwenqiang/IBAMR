// Filename FESurfaceDistanceEvaluator.h
// Created on Sep 5, 2018 by Nishant Nangia and Amneet Bhalla
//
// Copyright (c) 2002-2018, Nishant Nangia and Amneet Bhalla
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of The University of North Carolina nor the names of
//      its contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE

/////////////////////// INCLUDE GUARD ////////////////////////////////////

#ifndef included_FESurfaceDistanceEvaluator
#define included_FESurfaceDistanceEvaluator

///////////////////////////// INCLUDES ///////////////////////////////////
#include <map>
#include <vector>

#include "PatchHierarchy.h"
#include "ibamr/IBFEMethod.h"
#include "ibtk/IndexUtilities.h"
#include "ibtk/ibtk_utilities.h"
#include "ibtk/libmesh_utilities.h"
#include "libmesh/boundary_mesh.h"
#include "tbox/Pointer.h"

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief class FESurfaceDistanceEvaluator is a utility class which is used to identify
 * which line (triangle) in 2D (3D) elements are intersecting which grid cells.
 */
class FESurfaceDistanceEvaluator
{
public:
    /*!
     *  \brief Set uninitialized distance value to \em s_large_distance.
     *
     *  \note Since the class computes distance from the interface only upto
     *  ghost width specified in the constructor of the class, the rest of the
     *  distance values are uninitialized. It is advisable to set a specific number
     *  to distinguish between initialized and uninitialzed distance values.
     *
     *  \note An unitialized distance value of zero, could lead to subtle bugs in the
     *  code as zero distance value implies cell centers intersected by the interface.
     *
     */
    static const double s_large_distance;

    /*!
     * \brief The only constructor of this class.
     *
     * \param mesh The finite element mesh representing either codim-0 or codim-1 object.
     *
     * \param bdry_mesh The codim-1 version of \em mesh. If provided \em mesh is already codim-1, then
     * \em bdry_mesh will be equivalent to \em \mesh.

     * \param gcw These are the number of cells on inside and outside region demarcated by the interface
     * for which ditance is computed.
     *
     * \param use_extracted_bdry_mesh Boolean indicating if we are working with codim-1 mesh that is extracted
     * from a codim-0 \em mesh. Therefore, this boolean should be true for codim-0 \em mesh and false
     * for codim-1 \em mesh.
     */
    FESurfaceDistanceEvaluator(const std::string& object_name,
                               SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > patch_hierarchy,
                               SAMRAI::tbox::Pointer<IBAMR::IBFEMethod> ibfe_method,
                               const libMesh::Mesh& mesh,
                               const libMesh::BoundaryMesh& bdry_mesh,
                               const int part,
                               const int gcw = 1,
                               bool use_extracted_bdry_mesh = true);

    /*!
     * \brief Destructor for this class.
     */
    ~FESurfaceDistanceEvaluator();

    /*!
     * \brief Map the triangles intersecting a particular grid cell.
     */
    void mapIntersections();

    /*!
     * \brief Get the map maintaining triangle-cell intersection and neighbors.
     */
    const std::map<SAMRAI::pdat::CellIndex<NDIM>, std::set<libMesh::Elem*>, IBTK::CellIndexFortranOrder>&
    getNeighborIntersectionsMap();

    /*!
     * \brief Compute the signed distance in the viscinity of the finite element mesh.
     *
     * \param n_idx Patch data index to hold number of finite elements in Cartesian cell.
     *  A negative patch data index ignores populating this field.
     *
     * \param d_idx Patch data index to hold the distance value from the interface.
     *
     * \note The user should first initialize d_idx with (positive and uniform) "large" distance value
     * (relevant to a particular application) away from the interface before letting this function
     * calculate distance values near the interface.
     */
    void computeSignedDistance(int n_idx, int d_idx);

    /*!
     * \brief Update the sign of the \em large_distance value away from the finite element mesh.
     *
     * \note Some specified distance (as given in the constructor of this class) away from the interface,
     * the distance value is set to (positive) \em large_distance. This routine negates \em large_distance
     * value inside the body, while retaining its positive sign outside the body.
     */
    static void updateSignAwayFromInterface(int d_idx,
                                            SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > patch_hierarchy,
                                            double large_distance = s_large_distance);

    /*!
     * Check whether the grown box and line element intersect in 2D.
     *
     *     (box_tl)   *--------------*  (box_tr)
     *                |              |
     *                |              |
     *                |              |
     *                |              |
     *                |              |
     *     (box_bl)   *--------------*  (box_br)
     */
    static bool checkIntersection2D(const IBTK::Vector3d& box_bl,
                                    const IBTK::Vector3d& box_tr,
                                    const IBTK::Vector3d& box_br,
                                    const IBTK::Vector3d& box_tl,
                                    const libMesh::Point& n0,
                                    const libMesh::Point& n1);

    /*!
     * Check whether the grown box and triangle element intersect in 3D.
     *
     *                  ^   --------------
     *                  |  |              |                  vert0
     *  box_half_dx[1]  |  |              |                    *
     *                  |  |              |                   / \
     *                  ^  |      *       |                  /   \
     *                     |   box_center |                 /     \
     *                     |              |                /       \
     *                     |              |               /         \
     *                      --------------               *-----------*
     *                           <------->
     *                            box_half_dx[0]      vert1         vert2
     */
    static bool checkIntersection3D(const IBTK::Vector3d& box_center,
                                    const IBTK::Vector3d& box_half_dx,
                                    const IBTK::Vector3d& vert0,
                                    const IBTK::Vector3d& vert1,
                                    const IBTK::Vector3d& vert2);

    /*!
     * Get the closest point for a given point and list of triangle vertices.
     */
    static IBTK::Vector3d getClosestPoint3D(const IBTK::Vector3d& P,
                                            const libMesh::Point& vert0,
                                            const libMesh::Point& vert1,
                                            const libMesh::Point& vert2);

    //////////////// PRIVATE /////////////////////////////

private:
    /*!
     * Default constructor is not implemented and should not be used.
     */
    FESurfaceDistanceEvaluator();

    /*!
     * Default assignment operator is not implemented and should not be used.
     */
    FESurfaceDistanceEvaluator& operator=(const FESurfaceDistanceEvaluator& that);

    /*!
     * Default copy constructor is not implemented and should not be used.
     */
    FESurfaceDistanceEvaluator(const FESurfaceDistanceEvaluator& from);

    /*!
     * Collect all of the neighboring elements which are located within a local
     * Cartesian grid patch grown by the specified ghost cell width.
     *
     * In this method, the determination as to whether an element is local or
     * not is based on the physical location of its nodes and centroid.
     */
    void collectNeighboringPatchElements(int level_number);

    /*!
     * Compute signed distance from volume extracted boundary mesh.
     */
    void computeSignedDistanceVolExtractedBdryMesh(int n_idx, int d_idx);

    /*!
     * Compute signed distance from surface mesh.
     */
    void computeSignedDistanceSurfaceMesh(int n_idx, int d_idx);

    /*!
     * Name of this object.
     */
    std::string d_object_name;

    /*!
     * Pointer to Patch Hierarchy.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_patch_hierarchy;

    /*!
     * Pointer to the FEDataManager for a particular part.
     */
    SAMRAI::tbox::Pointer<IBAMR::IBFEMethod> d_ibfe_method;

    /*!
     * Volume mesh object
     */
    const libMesh::Mesh& d_mesh;

    /*!
     * Boundary mesh object
     */
    const libMesh::BoundaryMesh& d_bdry_mesh;

    /*!
     * The part number.
     */
    int d_part;

    /*!
     * The desired ghost cell width.
     */
    int d_gcw;

    /*!
     * Check if we are using volume extracted boundary mesh.
     */
    bool d_use_vol_extracted_bdry_mesh;

    /*!
     * The supported element type for this class
     */
    libMesh::ElemType d_supported_elem_type;

    /*!
     * Data to manage mapping between boundary mesh elements and grid patches.
     */
    std::vector<std::vector<libMesh::Elem*> > d_active_neighbor_patch_bdry_elem_map;

    /*!
     * Map object keeping track of element-cell intersections as well as elements intersecting that cell
     * and its neighboring cells within the ghost cell width. Note that the elements contained in thie
     * map belong to the original solid mesh.
     */
    std::map<SAMRAI::pdat::CellIndex<NDIM>, std::set<libMesh::Elem*>, IBTK::CellIndexFortranOrder>
        d_cell_elem_neighbor_map;
};

} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif // #ifndef included_FESurfaceDistanceEvaluator
