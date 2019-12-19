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

/////////////////////// INCLUDE GUARD ////////////////////////////////////

#ifndef included_IrregularWaveGenerator
#define included_IrregularWaveGenerator

///////////////////////////// INCLUDES ///////////////////////////////////

#include "ibamr/StokesWaveGeneratorStrategy.h"

namespace IBAMR
{
/*!
 * \brief Class for generating Irregular waves.
 */
class IrregularWaveGenerator : public StokesWaveGeneratorStrategy
{
public:
    IrregularWaveGenerator(const std::string& object_name, SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db);

    /*!
     * Get surface elevation at a specified horizontal position and time.
     */
    double getSurfaceElevation(double x, double time) const;

    /*!
     * Get velocity component at a specified position and time.
     */
    double getVelocity(double x, double z_plus_d, double time, int comp_idx) const;

private:
    /*!
     * Get wave parameters from input db.
     */
    void getFromInput(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

    /*!
     * \brief Wave parameters.
     *
     * \param d_num_waves       : Number of component waves with random phases to be generated (default = 50)
     * \param d_depth           : Depth of water, from sea bed to still water level [$m$]
     * \param d_Hs              : Significant wave height [$m$]
     * \param d_Ts              : significant wave period [$s$]
     * \param d_omega_begin     : Lowest angular frequency in the spectrum [$rad/s$]
     * \param d_omega_end       : Highest angular frequency in the spectrum [$rad/s$]
     * \param d_wave_spectrum   : JONSWAP/Bretschneider wave spectrum.
     * \param d_omega           : Angular frequencies of component waves [$rad/s$]
     * \param d_wave_number     : Wave number of component waves [$2\pi/m$]
     * \param d_amplitude       : Amplitude of component waves [$m$]
     * \param d_phase           : Phase (random) of component waves [$rad$]
     */
    int d_num_waves = 50; // default value is set to 50
    double d_omega_begin, d_omega_end, d_Ts, d_Hs;
    std::string d_wave_spectrum;
    std::vector<double> d_omega, d_wave_number, d_amplitude, d_phase;
};

} // namespace IBAMR
#endif
