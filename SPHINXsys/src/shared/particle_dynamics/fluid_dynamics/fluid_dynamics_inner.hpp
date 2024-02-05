/**
 * @file 	fluid_dynamics_inner.hpp
 * @author	Chi ZHang and Xiangyu Hu
 */

#pragma once

#include "fluid_dynamics_inner.h"

//=================================================================================================//
namespace SPH
{
	//=================================================================================================//
	namespace fluid_dynamics
	{
		//=================================================================================================//
		template <class RiemannSolverType>
		BasePressureRelaxationInner<RiemannSolverType>::
			BasePressureRelaxationInner(BaseBodyRelationInner &inner_relation)
			: BasePressureRelaxation(inner_relation),
			  riemann_solver_(fluid_, fluid_) {}
		//=================================================================================================//
		template <class RiemannSolverType>
		void BasePressureRelaxationInner<RiemannSolverType>::interaction(size_t index_i, Real dt)
		{
			Vecd acceleration(0);
			Real rho_dissipation(0);
			const Neighborhood &inner_neighborhood = inner_configuration_[index_i];
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				size_t index_j = inner_neighborhood.j_[n];
				Real dW_ijV_j = inner_neighborhood.dW_ijV_j_[n];
				const Vecd &e_ij = inner_neighborhood.e_ij_[n];

				acceleration -= (p_[index_i] + p_[index_j]) * dW_ijV_j * e_ij;
				rho_dissipation += riemann_solver_.getEffectiveVJump(p_[index_i], p_[index_j]) * dW_ijV_j;
			}
			acc_[index_i] += acceleration / rho_[index_i];
			drho_dt_[index_i] = 0.5 * rho_dissipation * rho_[index_i];
		}
		//=================================================================================================//
		template <class RiemannSolverType>
		BaseDensityRelaxationInner<RiemannSolverType>::
			BaseDensityRelaxationInner(BaseBodyRelationInner &inner_relation)
			: BaseDensityRelaxation(inner_relation),
			  riemann_solver_(fluid_, fluid_) {}
		//=================================================================================================//
		template <class RiemannSolverType>
		void BaseDensityRelaxationInner<RiemannSolverType>::interaction(size_t index_i, Real dt)
		{
			Real density_change_rate(0);
			Vecd p_dissipation(0);
			const Neighborhood &inner_neighborhood = inner_configuration_[index_i];
			for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
			{
				size_t index_j = inner_neighborhood.j_[n];
				const Vecd &e_ij = inner_neighborhood.e_ij_[n];
				Real dW_ijV_j = inner_neighborhood.dW_ijV_j_[n];

				density_change_rate += dot(vel_[index_i] - vel_[index_j], e_ij) * dW_ijV_j;
				p_dissipation -= riemann_solver_.getEffectivePJump(vel_[index_i], vel_[index_j], e_ij) * dW_ijV_j * e_ij;
			}
			drho_dt_[index_i] += density_change_rate * rho_[index_i];
			acc_[index_i] = 0.5 * p_dissipation / rho_[index_i];
		};
		//=================================================================================================//
	}
	//=================================================================================================//
}
//=================================================================================================//