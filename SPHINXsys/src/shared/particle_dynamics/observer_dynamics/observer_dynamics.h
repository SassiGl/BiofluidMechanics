/* -------------------------------------------------------------------------*
*								SPHinXsys									*
* --------------------------------------------------------------------------*
* SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle	*
* Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
* physical accurate simulation and aims to model coupled industrial dynamic *
* systems including fluid, solid, multi-body dynamics and beyond with SPH	*
* (smoothed particle hydrodynamics), a meshless computational method using	*
* particle discretization.													*
*																			*
* SPHinXsys is partially funded by German Research Foundation				*
* (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1				*
* and HU1527/12-1.															*
*                                                                           *
* Portions copyright (c) 2017-2020 Technical University of Munich and		*
* the authors' affiliations.												*
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License"); you may   *
* not use this file except in compliance with the License. You may obtain a *
* copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
*                                                                           *
* --------------------------------------------------------------------------*/
/**
 * @file 	observer_dynamics.h
 * @brief 	There are the classes for observser bodies to record the state of the flow or
 *			solid in given locations. Mostly, this is done by an interpolation algorithm.   
 * @author	Xiangyu Hu and Chi Zhang
 */


#ifndef OBSERVER_DYNAMICS_H
#define OBSERVER_DYNAMICS_H



#include "all_particle_dynamics.h"

namespace SPH
{
	namespace observer_dynamics
	{
		typedef DataDelegateContact<SPHBody, BaseParticles, BaseMaterial, 
			SPHBody, BaseParticles, BaseMaterial> InterpolationContactData;

		/**
		 * @class InterpolatingAQuantity
		 * @brief Interpolate a given member data in the particles of a general body
		 */
		template <int DataTypeIndex, typename VariableType>
		class InterpolatingAQuantity : public InteractionDynamics, public InterpolationContactData
		{
		public:
			explicit InterpolatingAQuantity(BaseBodyRelationContact* body_contact_relation, std::string variable_name) :
				InteractionDynamics(body_contact_relation->sph_body_), InterpolationContactData(body_contact_relation),
				interpolated_quantities_(*particles_->createAVariable<DataTypeIndex, VariableType>(variable_name))
			{
				prepareContactData(variable_name);
			};
			explicit InterpolatingAQuantity(BaseBodyRelationContact* body_contact_relation,
				std::string interpolated_variable, std::string target_variable) :
				InteractionDynamics(body_contact_relation->sph_body_), InterpolationContactData(body_contact_relation),
				interpolated_quantities_(*particles_->getVariableByName<DataTypeIndex, VariableType>(interpolated_variable))
			{
				prepareContactData(target_variable);
			};
			virtual ~InterpolatingAQuantity() {};

		protected:
			StdLargeVec<VariableType>&  interpolated_quantities_;
			StdVec<StdLargeVec<Real>*> contact_Vol_;
			StdVec<StdLargeVec<VariableType>*> contact_data_;

			virtual void Interaction(size_t index_i, Real dt = 0.0) override
			{
				VariableType observed_quantity(0);
				Real ttl_weight(0);

				for (size_t k = 0; k < this->contact_configuration_.size(); ++k)
				{
					StdLargeVec<Real>& Vol_k = *(contact_Vol_[k]);
					StdLargeVec<VariableType>& data_k = *(contact_data_[k]);
					Neighborhood& contact_neighborhood = (*this->contact_configuration_[k])[index_i];
					for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
					{
						size_t index_j = contact_neighborhood.j_[n];
						Real weight_j = contact_neighborhood.W_ij_[n] * Vol_k[index_j];

						observed_quantity += weight_j * data_k[index_j];
						ttl_weight += weight_j;
					}
				}
				interpolated_quantities_[index_i] = observed_quantity / (ttl_weight + TinyReal);
			};

			void prepareContactData(const std::string& variable_name)
			{
				for (size_t k = 0; k != this->contact_particles_.size(); ++k)
				{
					contact_Vol_.push_back(&(this->contact_particles_[k]->Vol_));
					StdLargeVec<VariableType>* contact_data =
						this->contact_particles_[k]->template getVariableByName<DataTypeIndex, VariableType>(variable_name);
					contact_data_.push_back(contact_data);
				}
			}
		};

		/**
		* @class CorrectInterpolationKernelWeights
		* @brief  correct kernel weights for interpolation between general bodies
		*/
		class CorrectInterpolationKernelWeights : 
			public InteractionDynamics, public InterpolationContactData
		{
		public:
			CorrectInterpolationKernelWeights(BaseBodyRelationContact* body_contact_relation);
			virtual ~CorrectInterpolationKernelWeights() {};
		protected:
			StdVec<StdLargeVec<Real>*> contact_Vol_;
			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
		};
	}
}
#endif //OBSERVER_DYNAMICS_H