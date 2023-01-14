/* -------------------------------------------------------------------------*
 *								SPHinXsys									*
 * -------------------------------------------------------------------------*
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle*
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
 * physical accurate simulation and aims to model coupled industrial dynamic*
 * systems including fluid, solid, multi-body dynamics and beyond with SPH	*
 * (smoothed particle hydrodynamics), a meshless computational method using	*
 * particle discretization.													*
 *																			*
 * SPHinXsys is partially funded by German Research Foundation				*
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,			*
 *  HU1527/12-1 and HU1527/12-4													*
 *                                                                          *
 * Portions copyright (c) 2017-2022 Technical University of Munich and		*
 * the authors' affiliations.												*
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may  *
 * not use this file except in compliance with the License. You may obtain a*
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.       *
 *                                                                          *
 * ------------------------------------------------------------------------*/
/**
 * @file 	base_body_relation.h
 * @brief 	Base classes on body and particle topology relations.
 * @author	Chi ZHang and Xiangyu Hu
 */

#ifndef BASE_BODY_RELATION_H
#define BASE_BODY_RELATION_H

#include "complex_body.h"
#include "base_particles.h"
#include "cell_linked_list.h"
#include "neighborhood.h"
#include "base_geometry.h"

namespace SPH
{
	/** a small functor for obtaining search range for the simplest case */
	struct SearchDepthSingleResolution
	{
		int operator()(size_t particle_index) const { return 1; };
	};

	/** @brief a small functor for obtaining search depth across resolution
	 * @details Note that the search depth is defined on the target cell linked list.
	 */
	struct SearchDepthContact
	{
		int search_depth_;
		SearchDepthContact(SPHBody &sph_body, CellLinkedList *target_cell_linked_list)
			: search_depth_(1)
		{
			Real inv_grid_spacing_ = 1.0 / target_cell_linked_list->GridSpacing();
			Kernel *kernel_ = sph_body.sph_adaptation_->getKernel();
			search_depth_ = 1 + (int)floor(kernel_->CutOffRadius() * inv_grid_spacing_);
		};
		int operator()(size_t particle_index) const { return search_depth_; };
	};

	/** @brief a small functor for obtaining search depth for variable smoothing length
	 * @details Note that the search depth is defined on the target cell linked list.
	 */
	struct SearchDepthAdaptive
	{
		Real inv_grid_spacing_;
		Kernel *kernel_;
		StdLargeVec<Real> &h_ratio_;
		SearchDepthAdaptive(SPHBody &sph_body, CellLinkedList *target_cell_linked_list)
			: inv_grid_spacing_(1.0 / target_cell_linked_list->GridSpacing()),
			  kernel_(sph_body.sph_adaptation_->getKernel()),
			  h_ratio_(*sph_body.getBaseParticles().getVariableByName<Real>("SmoothingLengthRatio")){};
		int operator()(size_t particle_index) const
		{
			return 1 + (int)floor(kernel_->CutOffRadius(h_ratio_[particle_index]) * inv_grid_spacing_);
		};
	};

	/** @brief a small functor for obtaining search depth for variable smoothing length
	 * @details Note that this is only for building contact neighbor relation.
	 */
	struct SearchDepthAdaptiveContact
	{
		Real inv_grid_spacing_;
		SPHAdaptation &sph_adaptation_;
		Kernel &kernel_;
		SearchDepthAdaptiveContact(SPHBody &sph_body, CellLinkedList *target_cell_linked_list)
			: inv_grid_spacing_(1.0 / target_cell_linked_list->GridSpacing()),
			  sph_adaptation_(*sph_body.sph_adaptation_),
			  kernel_(*sph_body.sph_adaptation_->getKernel()){};
		int operator()(size_t particle_index) const
		{
			return 1 + (int)floor(kernel_.CutOffRadius(sph_adaptation_.SmoothingLengthRatio(particle_index)) * inv_grid_spacing_);
		};
	};

	/** Transfer body parts to real bodies. **/
	RealBodyVector BodyPartsToRealBodies(BodyPartVector body_parts);

	/**
	 * @class SPHRelation
	 * @brief The abstract class for all relations within a SPH body or with its contact SPH bodies
	 */
	class SPHRelation
	{
	public:
		SPHBody &sph_body_;
		BaseParticles &base_particles_;
		SPHBody &getDynamicsRange() { return sph_body_; };
		explicit SPHRelation(SPHBody &sph_body);
		virtual ~SPHRelation(){};
		bool isTotalLagrangian() { return is_total_lagrangian_; };
		virtual void updateConfigurationMemories() = 0;
		virtual void updateConfiguration() = 0;
		virtual void setUpdateCellLinkedList() = 0;

	protected:
		bool is_total_lagrangian_;
	};

	/**
	 * @class BaseInnerRelation
	 * @brief The abstract relation within a SPH body
	 */
	class BaseInnerRelation : public SPHRelation
	{
	public:
		RealBody *real_body_;
		ParticleConfiguration inner_configuration_; /**< inner configuration for the neighbor relations. */
		explicit BaseInnerRelation(RealBody &real_body);
		virtual ~BaseInnerRelation(){};
		virtual void updateConfigurationMemories() override;
		BaseInnerRelation &setTotalLagrangian();
		virtual void setUpdateCellLinkedList() override;

	protected:
		virtual void resetNeighborhoodCurrentSize();
	};

	/**
	 * @class BaseContactRelation
	 * @brief The base relation between a SPH body and its contact SPH bodies
	 */
	class BaseContactRelation : public SPHRelation
	{
	protected:
		virtual void resetNeighborhoodCurrentSize();

	public:
		RealBodyVector contact_bodies_;
		ContactParticleConfiguration contact_configuration_; /**< Configurations for particle interaction between bodies. */

		BaseContactRelation(SPHBody &sph_body, RealBodyVector contact_bodies);
		BaseContactRelation(SPHBody &sph_body, BodyPartVector contact_body_parts)
			: BaseContactRelation(sph_body, BodyPartsToRealBodies(contact_body_parts)){};
		virtual ~BaseContactRelation(){};
		virtual void updateConfigurationMemories() override;
		BaseContactRelation &setTotalLagrangian();
		virtual void setUpdateCellLinkedList() override;
	};

	/**
	 * @class ComplexRelation
	 * @brief The relation combined an inner and a contact body relation.
	 * TODO: it seems that this class is not necessary ?
	 */
	class ComplexRelation
	{
	protected:
		BaseInnerRelation &inner_relation_;
		BaseContactRelation &contact_relation_;

	public:
		BaseInnerRelation &getInnerRelation() { return inner_relation_; };
		BaseContactRelation &getContactRelation() { return contact_relation_; };
		SPHBody &getDynamicsRange() { return inner_relation_.sph_body_; };
		ComplexRelation(BaseInnerRelation &inner_relation, BaseContactRelation &contact_relation)
			: inner_relation_(inner_relation),
			  contact_relation_(contact_relation){};
		virtual ~ComplexRelation(){};
	};
}
#endif // BASE_BODY_RELATION_H