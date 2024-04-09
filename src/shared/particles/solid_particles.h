/* ------------------------------------------------------------------------- *
 *                                SPHinXsys                                  *
 * ------------------------------------------------------------------------- *
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle *
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for    *
 * physical accurate simulation and aims to model coupled industrial dynamic *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH   *
 * (smoothed particle hydrodynamics), a meshless computational method using  *
 * particle discretization.                                                  *
 *                                                                           *
 * SPHinXsys is partially funded by German Research Foundation               *
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,            *
 *  HU1527/12-1 and HU1527/12-4                                              *
 *                                                                           *
 * Portions copyright (c) 2017-2022 Technical University of Munich and       *
 * the authors' affiliations.                                                *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may   *
 * not use this file except in compliance with the License. You may obtain a *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
/**
 * @file 	solid_particles.h
 * @brief 	This is the derived class of base particles.
 * @author	Chi Zhang, Dong Wu and Xiangyu Hu
 */

#ifndef SOLID_PARTICLES_H
#define SOLID_PARTICLES_H

#include "base_particles.h"
#include "base_particles.hpp"
#include "elastic_solid.h"

#include "particle_generator_lattice.h"
namespace SPH
{
class Solid;
class ElasticSolid;

/**
 * @class SolidParticles
 * @brief A group of particles with solid body particle data.
 */
class SolidParticles : public BaseParticles
{
  public:
    SolidParticles(SPHBody &sph_body, Solid *solid);
    virtual ~SolidParticles(){};
    Solid &solid_;


    /** Initialized variables for solid particles. */
    virtual void initializeOtherVariables() override;
    /** Return this pointer. */
    virtual SolidParticles *ThisObjectPtr() override { return this; };
};

/**
 * @class ElasticSolidParticles
 * @brief A group of particles with elastic body particle data.
 */
class ElasticSolidParticles : public SolidParticles
{
  public:
    ElasticSolidParticles(SPHBody &sph_body, ElasticSolid *elastic_solid);
    virtual ~ElasticSolidParticles(){};
    ElasticSolid &elastic_solid_;

    /** Initialize the variables for elastic particle. */
    virtual void initializeOtherVariables() override;
    /** Return this pointer. */
    virtual ElasticSolidParticles *ThisObjectPtr() override { return this; };
};

/**
 * @class ShellParticles
 * @brief A group of particles with shell particle data.
 */
class ShellParticles : public ElasticSolidParticles
{
  public:
    ShellParticles(SPHBody &sph_body, ElasticSolid *elastic_solid);
    virtual ~ShellParticles(){};

    Real thickness_ref_;                      /**< Shell thickness. */
    StdLargeVec<Matd> transformation_matrix_; /**< initial transformation matrix from global to local coordinates */
    StdLargeVec<Real> thickness_;             /**< shell thickness */
    /**
     *	extra generalized coordinates in global coordinate
     */
    StdLargeVec<Vecd> pseudo_n_;      /**< current pseudo-normal vector */
    StdLargeVec<Vecd> dpseudo_n_dt_;  /**< pseudo-normal vector change rate */
    StdLargeVec<Vecd> dpseudo_n_d2t_; /**< pseudo-normal vector second order time derivation */
    /**
     *	extra generalized coordinate and velocity in local coordinate
     */
    StdLargeVec<Vecd> rotation_;        /**< rotation angle of the initial normal respective to each axis */
    StdLargeVec<Vecd> angular_vel_;     /**< angular velocity respective to each axis */
    StdLargeVec<Vecd> dangular_vel_dt_; /**< angular acceleration of respective to each axis*/
    /**
     *	extra deformation and deformation rate in local coordinate
     */
    StdLargeVec<Matd> F_bending_;     /**< bending deformation gradient	*/
    StdLargeVec<Matd> dF_bending_dt_; /**< bending deformation gradient change rate	*/
    /**
     *	extra stress for pair interaction in global coordinate
     */
    StdLargeVec<Vecd> global_shear_stress_; /**< global shear stress */
    StdLargeVec<Matd> global_stress_;       /**<  global stress for pair interaction */
    StdLargeVec<Matd> global_moment_;       /**<  global bending moment for pair interaction */
    /**
     *	extra stress for calculating von Mises stress of shell mid-surface
     */
    StdLargeVec<Matd> mid_surface_cauchy_stress_;
    /**
     *	extra scaling matrix fot numerical damping
     */
    StdLargeVec<Matd> numerical_damping_scaling_;

    /** get particle volume. */
    virtual Real ParticleVolume(size_t index_i) override { return Vol_[index_i] * thickness_[index_i]; }
    virtual void initializeOtherVariables() override;
    /** Return this pointer. */
    virtual ShellParticles *ThisObjectPtr() override { return this; };
};

} // namespace SPH
#endif // SOLID_PARTICLES_H
