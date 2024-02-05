#pragma once
#include "k-epsilon_turbulent_model.hpp"
namespace SPH
{
//=================================================================================================//
namespace fluid_dynamics
{
//=================================================================================================//
	BaseTurbuClosureCoeff::BaseTurbuClosureCoeff()
		: Karman(0.4187), C_mu(0.09), TurbulentIntensity(5.0e-2), sigma_k(1.0),
		C_l(1.44), C_2(1.92), sigma_E(1.3), turbu_const_E(9.793) {}
//=================================================================================================//
    void GetVelocityGradient<Inner<>>::interaction(size_t index_i, Real dt)
    {
		//** The near wall velo grad is updated in wall function part *
		if (is_near_wall_P1_[index_i] == 1)
		{
			return;
		}
		Vecd vel_i = vel_[index_i];
		velocity_gradient_[index_i] = Matd::Zero();
		const Neighborhood& inner_neighborhood = inner_configuration_[index_i];
		for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
		{
			size_t index_j = inner_neighborhood.j_[n];
			Vecd nablaW_ijV_j = inner_neighborhood.dW_ijV_j_[n] * inner_neighborhood.e_ij_[n];
			//** Strong form *
			velocity_gradient_[index_i] += -(vel_i - vel_[index_j]) * nablaW_ijV_j.transpose();
			//** Weak form *
			//velocity_gradient_[index_i] += (vel_i + vel_[index_j]) * nablaW_ijV_j.transpose();
		}
    }
//=================================================================================================//
	K_TurtbulentModelInner::K_TurtbulentModelInner(BaseInnerRelation& inner_relation, const StdVec<Real>& initial_values)
		: BaseTurtbulentModel<Base, FluidDataInner>(inner_relation)
	{
		//** Obtain Initial values for transport equations *
		turbu_k_initial_ = initial_values[0];
		turbu_ep_initial_ = initial_values[1];
		turbu_mu_initial_ = initial_values[2];

		particles_->registerVariable(dk_dt_, "ChangeRateOfTKE");
		particles_->registerSortableVariable<Real>("ChangeRateOfTKE");

		particles_->registerVariable(turbu_k_, "TurbulenceKineticEnergy", turbu_k_initial_);

		particles_->registerSortableVariable<Real>("TurbulenceKineticEnergy");
		particles_->addVariableToWrite<Real>("TurbulenceKineticEnergy");

		particles_->registerVariable(turbu_mu_, "TurbulentViscosity", turbu_mu_initial_);
		particles_->registerSortableVariable<Real>("TurbulentViscosity");
		particles_->addVariableToWrite<Real>("TurbulentViscosity");

		particles_->registerVariable(turbu_epsilon_, "TurbulentDissipation", turbu_ep_initial_);

		particles_->registerSortableVariable<Real>("TurbulentDissipation");
		particles_->addVariableToWrite<Real>("TurbulentDissipation");

		particles_->registerVariable(k_production_, "K_Production");
		particles_->registerSortableVariable<Real>("K_Production");
		particles_->addVariableToWrite<Real>("K_Production");

		particles_->registerVariable(is_near_wall_P1_, "IsNearWallP1");
		particles_->registerSortableVariable<int>("IsNearWallP1");
		particles_->addVariableToWrite<int>("IsNearWallP1");

		//** for test */
		particles_->registerVariable(k_diffusion_, "K_Diffusion");
		particles_->registerSortableVariable<Real>("K_Diffusion");
		particles_->addVariableToWrite<Real>("K_Diffusion");

		particles_->addVariableToWrite<Real>("ChangeRateOfTKE");

		particles_->registerVariable(velocity_gradient_, "VelocityGradient");
		particles_->registerSortableVariable<Matd>("VelocityGradient");
		particles_->addVariableToWrite<Matd>("VelocityGradient");

		particles_->registerVariable(vel_x_, "Velocity_X");
		particles_->registerSortableVariable<Real>("Velocity_X");
	}
	//=================================================================================================//
	void K_TurtbulentModelInner::interaction(size_t index_i, Real dt)
	{
		Vecd vel_i = vel_[index_i];
		Real rho_i = rho_[index_i];
		Real turbu_mu_i = turbu_mu_[index_i];
		Real turbu_k_i = turbu_k_[index_i];

		Real mu_eff_i = turbu_mu_[index_i] / sigma_k + mu_;

		dk_dt_[index_i] = 0.0;;
		Real k_derivative(0.0);
		Real k_lap(0.0);
		Matd strain_rate = Matd::Zero();
		Matd Re_stress = Matd::Zero();

		const Neighborhood& inner_neighborhood = inner_configuration_[index_i];
		for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
		{
			size_t index_j = inner_neighborhood.j_[n];
			Real mu_eff_j = turbu_mu_[index_j] / sigma_k + mu_;
			Real mu_harmo = 2 * mu_eff_i * mu_eff_j / (mu_eff_i + mu_eff_j);
			k_derivative = (turbu_k_i - turbu_k_[index_j]) / (inner_neighborhood.r_ij_[n] + 0.01 * smoothing_length_);
			k_lap += 2.0 * mu_harmo * k_derivative * inner_neighborhood.dW_ijV_j_[n] / rho_i;
		}
		strain_rate = 0.5 * (velocity_gradient_[index_i].transpose() + velocity_gradient_[index_i]);

		Re_stress = 2.0 * strain_rate * turbu_mu_i / rho_i - (2.0 / 3.0) * turbu_k_i * Matd::Identity();
		//Re_stress = 2.0 * strain_rate * turbu_mu_i / rho_i;

		Matd k_production_matrix = Re_stress.array() * velocity_gradient_[index_i].array();
		//** The near wall k production is updated in wall function part *
		if (is_near_wall_P1_[index_i] != 1)
			k_production_[index_i] = k_production_matrix.sum();

		dk_dt_[index_i] = k_production_[index_i] - turbu_epsilon_[index_i] + k_lap;

		//** for test */
		k_diffusion_[index_i] = k_lap;
		vel_x_[index_i] = vel_[index_i][0];
	}
	//=================================================================================================//
	void K_TurtbulentModelInner::update(size_t index_i, Real dt)
	{
		turbu_k_[index_i] += dk_dt_[index_i] * dt;
	}
//=================================================================================================//
	E_TurtbulentModelInner::E_TurtbulentModelInner(BaseInnerRelation& inner_relation)
		: BaseTurtbulentModel<Base, FluidDataInner>(inner_relation),
		k_production_(*particles_->getVariableByName<Real>("K_Production")),
		turbu_k_(*particles_->getVariableByName<Real>("TurbulenceKineticEnergy")),
		turbu_mu_(*particles_->getVariableByName<Real>("TurbulentViscosity")),
		turbu_epsilon_(*particles_->getVariableByName<Real>("TurbulentDissipation"))
	{
		particles_->registerVariable(dE_dt_, "ChangeRateOfTDR");
		particles_->registerSortableVariable<Real>("ChangeRateOfTDR");
		particles_->addVariableToWrite<Real>("ChangeRateOfTDR");

		particles_->registerVariable(ep_production, "Ep_Production");
		particles_->registerSortableVariable<Real>("Ep_Production");
		particles_->addVariableToWrite<Real>("Ep_Production");
		particles_->registerVariable(ep_dissipation_, "Ep_Dissipation_");
		particles_->registerSortableVariable<Real>("Ep_Dissipation_");
		particles_->addVariableToWrite<Real>("Ep_Dissipation_");
		particles_->registerVariable(ep_diffusion_, "Ep_Diffusion_");
		particles_->registerSortableVariable<Real>("Ep_Diffusion_");
		particles_->addVariableToWrite<Real>("Ep_Diffusion_");
	}
	//=================================================================================================//
	void E_TurtbulentModelInner::
		interaction(size_t index_i, Real dt)
	{
		Real rho_i = rho_[index_i];
		Real turbu_mu_i = turbu_mu_[index_i];
		Real turbu_k_i = turbu_k_[index_i];
		Real turbu_epsilon_i = turbu_epsilon_[index_i];

		Real mu_eff_i = turbu_mu_[index_i] / sigma_E + mu_;

		dE_dt_[index_i] = 0.0;
		Real epsilon_production(0.0);
		Real epsilon_derivative(0.0);
		Real epsilon_lap(0.0);
		Real epsilon_dissipation(0.0);
		const Neighborhood& inner_neighborhood = inner_configuration_[index_i];
		for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
		{
			size_t index_j = inner_neighborhood.j_[n];
			Real mu_eff_j = turbu_mu_[index_j] / sigma_E + mu_;
			Real mu_harmo = 2 * mu_eff_i * mu_eff_j / (mu_eff_i + mu_eff_j);
			epsilon_derivative = (turbu_epsilon_i - turbu_epsilon_[index_j]) / (inner_neighborhood.r_ij_[n] + 0.01 * smoothing_length_);
			epsilon_lap += 2.0 * mu_harmo * epsilon_derivative * inner_neighborhood.dW_ijV_j_[n] / rho_i;
		}

		epsilon_production = C_l * turbu_epsilon_i * k_production_[index_i] / turbu_k_i;
		epsilon_dissipation = C_2 * turbu_epsilon_i * turbu_epsilon_i / turbu_k_i;

		dE_dt_[index_i] = epsilon_production - epsilon_dissipation + epsilon_lap;

		//** for test */
		ep_production[index_i] = epsilon_production;
		ep_dissipation_[index_i] = epsilon_dissipation;
		ep_diffusion_[index_i] = epsilon_lap;
	}
	//=================================================================================================//
	void E_TurtbulentModelInner::update(size_t index_i, Real dt)
	{
		turbu_epsilon_[index_i] += dE_dt_[index_i] * dt;
	}
//=================================================================================================//
	TKEnergyForce<Inner<>>::TKEnergyForce(BaseInnerRelation& inner_relation)
		: TKEnergyForce<Base, FluidDataInner>(inner_relation),
		test_k_grad_rslt_(*this->particles_->template getVariableByName<Vecd>("TkeGradResult"))
	{
		this->particles_->registerVariable(tke_acc_inner_, "TkeAccInner");
		this->particles_->addVariableToWrite<Vecd>("TkeAccInner");
	}
	//=================================================================================================//
    void TKEnergyForce<Inner<>>::interaction(size_t index_i, Real dt)
    {
		Real turbu_k_i = turbu_k_[index_i];
		Vecd force = Vecd::Zero();
		Vecd k_gradient = Vecd::Zero();
		const Neighborhood& inner_neighborhood = inner_configuration_[index_i];
		for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
		{
			size_t index_j = inner_neighborhood.j_[n];
			Vecd nablaW_ijV_j = inner_neighborhood.dW_ijV_j_[n] * inner_neighborhood.e_ij_[n];
			//** strong form * 
			//k_gradient += -1.0*(turbu_k_i - turbu_k_[index_j]) * nablaW_ijV_j;
			//** weak form * 
			k_gradient += (turbu_k_i + turbu_k_[index_j]) * nablaW_ijV_j;
		}
		force = -1.0 * (2.0 / 3.0) * k_gradient * mass_[index_i];
		force_[index_i] += force;

		//for test
		tke_acc_inner_[index_i] = force / mass_[index_i];
		test_k_grad_rslt_[index_i] = k_gradient;
    }
	//=================================================================================================//
	TKEnergyForce<Contact<>>::TKEnergyForce(BaseContactRelation& contact_relation)
		: TKEnergyForce<Base, FluidContactData>(contact_relation),
		test_k_grad_rslt_(*this->particles_->template getVariableByName<Vecd>("TkeGradResult"))
	{
		this->particles_->registerVariable(tke_acc_wall_, "TkeAccWall");
		this->particles_->addVariableToWrite<Vecd>("TkeAccWall");
	}
	//=================================================================================================//
	void TKEnergyForce<Contact<>>::interaction(size_t index_i, Real dt)
	{
		Real turbu_k_i = turbu_k_[index_i];
		Vecd force = Vecd::Zero();
		Vecd k_gradient = Vecd::Zero();
		for (size_t k = 0; k < FluidContactData::contact_configuration_.size(); ++k)
		{
			Neighborhood& contact_neighborhood = (*FluidContactData::contact_configuration_[k])[index_i];
			for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
			{
				size_t index_j = contact_neighborhood.j_[n];
				Vecd nablaW_ijV_j = contact_neighborhood.dW_ijV_j_[n] * contact_neighborhood.e_ij_[n];
				//** weak form * 
				k_gradient +=  (turbu_k_i + turbu_k_i) * nablaW_ijV_j;
			}
		}
		force = -1.0 * (2.0 / 3.0) * k_gradient * mass_[index_i];
		force_[index_i] += force;

		//for test
		tke_acc_wall_[index_i] = force / mass_[index_i];
		test_k_grad_rslt_[index_i] += k_gradient;
	}
//=================================================================================================//
	TurbuViscousForce<Inner<>>::TurbuViscousForce(BaseInnerRelation& inner_relation)
		: TurbuViscousForce<FluidDataInner>(inner_relation),
		ForcePrior(&base_particles_, "ViscousForce")
	{
		this->particles_->registerVariable(visc_acc_inner_, "ViscousAccInner");
		this->particles_->addVariableToWrite<Vecd>("ViscousAccInner");
	}
//=================================================================================================//
	void TurbuViscousForce<Inner<>>::interaction(size_t index_i, Real dt)
	{
		Real mu_eff_i = turbu_mu_[index_i] + mu_;
		Vecd force = Vecd::Zero();
		Vecd vel_derivative = Vecd::Zero();
		const Neighborhood& inner_neighborhood = inner_configuration_[index_i];

		for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
		{
			size_t index_j = inner_neighborhood.j_[n];
			const Vecd& e_ij = inner_neighborhood.e_ij_[n];

			Real mu_eff_j = turbu_mu_[index_j] + mu_;
			Real mu_harmo = 2 * mu_eff_i * mu_eff_j / (mu_eff_i + mu_eff_j);
			vel_derivative = (vel_[index_i] - vel_[index_j]) / (inner_neighborhood.r_ij_[n] + 0.01 * smoothing_length_);

			Vecd force_j = 2.0 * mass_[index_i] * mu_harmo * vel_derivative * inner_neighborhood.dW_ijV_j_[n];
			force += force_j;
		}
		viscous_force_[index_i] = force / rho_[index_i];
		//for test
		visc_acc_inner_[index_i] = force / rho_[index_i]/ mass_[index_i];
	}
//=================================================================================================//
	TurbuViscousForce<Contact<Wall>>::TurbuViscousForce(BaseContactRelation& wall_contact_relation)
		: BaseTurbuViscousAccelerationWithWall(wall_contact_relation)
	{
		this->particles_->registerVariable(visc_acc_wall_, "ViscousAccWall");
		this->particles_->addVariableToWrite<Vecd>("ViscousAccWall");
	}
//=================================================================================================//
	Real TurbuViscousForce<Contact<Wall>>::standard_wall_functon_for_wall_viscous(Real vel_t, Real k_p, Real y_p, Real rho)
	{
		Real velo_fric = 0.0;
		velo_fric = sqrt(abs(Karman * vel_t * pow(C_mu, 0.25) * pow(k_p, 0.5) /
			log(turbu_const_E * pow(C_mu, 0.25) * pow(k_p, 0.5) * y_p * rho / mu_)));
		return velo_fric;
	}
//=================================================================================================//
	void TurbuViscousForce<Contact<Wall>>::interaction(size_t index_i, Real dt)
	{
		Real turbu_k_i = this->turbu_k_[index_i];
		Real rho_i = this->rho_[index_i];
		const Vecd& vel_i = this->vel_[index_i];
		Real y_p = this->y_p_[index_i];

		Vecd force = Vecd::Zero();
		
		Vecd e_j_n = Vecd::Zero();
		Vecd e_j_tau = Vecd::Zero();
		Matd WSS_j_tn = Matd::Zero();  //** Wall shear stress of wall particle j on t-n plane *
		Matd WSS_j = Matd::Zero();  //** Wall shear stress of wall particle j on x-y plane *
		Matd Q = Matd::Zero();
		for (size_t k = 0; k < contact_configuration_.size(); ++k)
		{
			StdLargeVec<Vecd>& vel_ave_k = *(this->wall_vel_ave_[k]);
			Neighborhood& contact_neighborhood = (*contact_configuration_[k])[index_i];
			StdLargeVec<Vecd>& n_k = *(this->wall_n_[k]);
			for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
			{
				size_t index_j = contact_neighborhood.j_[n];
				Real r_ij = contact_neighborhood.r_ij_[n];
				Vecd& e_ij = contact_neighborhood.e_ij_[n];

				e_j_n = n_k[index_j];
				Q = getTransformationMatrix(e_j_n);

				//** Get tangential unit vector, temporarily only suitable for 2D*
				e_j_tau[0] = e_j_n[1];
				e_j_tau[1] = e_j_n[0] * (-1.0);

				//** Calculate the local friction velocity *
				Real vel_i_tau_mag = abs(vel_i.dot(e_j_tau));
				Real fric_vel_mag = this->standard_wall_functon_for_wall_viscous(vel_i_tau_mag, turbu_k_i, y_p, rho_i);

				//** Construct local wall shear stress, if this is on each wall particle j   *
				WSS_j_tn(0, 0) = 0.0;
				WSS_j_tn(0, 1) = rho_i * fric_vel_mag * fric_vel_mag * boost::qvm::sign(vel_i.dot(e_j_tau));
				WSS_j_tn(1, 0) = 0.0;
				WSS_j_tn(1, 1) = 0.0;
				
				//** Transform local wall shear stress to global   *
				WSS_j = Q.transpose() * WSS_j_tn * Q;
				Vecd force_j =  2.0 * mass_[index_i] * WSS_j * e_ij * contact_neighborhood.dW_ijV_j_[n] / rho_i;

				force += force_j;
			}
		}
		viscous_force_[index_i] += force;
		//** For test *
		this->visc_acc_wall_[index_i] = force/ mass_[index_i];
	}
//=================================================================================================//
	TurbulentEddyViscosity::
		TurbulentEddyViscosity(SPHBody& sph_body)
		: LocalDynamics(sph_body), FluidDataSimple(sph_body),
		rho_(particles_->rho_), wall_Y_star_(*particles_->getVariableByName<Real>("WallYstar")),
		wall_Y_plus_(*particles_->getVariableByName<Real>("WallYplus")),
		mu_(DynamicCast<Fluid>(this, particles_->getBaseMaterial()).ReferenceViscosity()),
		turbu_k_(*particles_->getVariableByName<Real>("TurbulenceKineticEnergy")),
		turbu_mu_(*particles_->getVariableByName<Real>("TurbulentViscosity")),
		turbu_epsilon_(*particles_->getVariableByName<Real>("TurbulentDissipation")) {}
	//=================================================================================================//
	void TurbulentEddyViscosity::update(size_t index_i, Real dt)
	{
		turbu_mu_[index_i] = rho_[index_i] * C_mu * turbu_k_[index_i] * turbu_k_[index_i] / (turbu_epsilon_[index_i]);
	}
//=================================================================================================//
	TurbulentAdvectionTimeStepSize::TurbulentAdvectionTimeStepSize(SPHBody& sph_body, Real U_max, Real advectionCFL)
		: LocalDynamicsReduce<Real, ReduceMax>(sph_body, U_max* U_max), FluidDataSimple(sph_body),
		vel_(particles_->vel_), advectionCFL_(advectionCFL),
		smoothing_length_min_(sph_body.sph_adaptation_->MinimumSmoothingLength()),
		fluid_(DynamicCast<Fluid>(this, particles_->getBaseMaterial())),
		turbu_mu_(*particles_->getVariableByName<Real>("TurbulentViscosity"))
	{
		Real viscous_speed = fluid_.ReferenceViscosity() / fluid_.ReferenceDensity() / smoothing_length_min_;
		reference_ = SMAX(viscous_speed * viscous_speed, reference_);
	}
	//=================================================================================================//
	Real TurbulentAdvectionTimeStepSize::reduce(size_t index_i, Real dt)
	{
		Real turbu_viscous_speed = (fluid_.ReferenceViscosity() + turbu_mu_[index_i])
			/ fluid_.ReferenceDensity() / smoothing_length_min_;
		Real turbu_viscous_speed_squre = turbu_viscous_speed * turbu_viscous_speed;
		Real vel_n_squre = vel_[index_i].squaredNorm();
		Real vel_bigger = SMAX(turbu_viscous_speed_squre, vel_n_squre);

		return vel_bigger;
	}
	//=================================================================================================//
	Real TurbulentAdvectionTimeStepSize::outputResult(Real reduced_value)
	{
		Real speed_max = sqrt(reduced_value);
		return advectionCFL_ * smoothing_length_min_ / (speed_max + TinyReal);
	}
//=================================================================================================//
	InflowTurbulentCondition::InflowTurbulentCondition(BodyPartByCell& body_part
		, Real CharacteristicLength, Real relaxation_rate) :
		BaseFlowBoundaryCondition(body_part),
		relaxation_rate_(relaxation_rate),
		CharacteristicLength_(CharacteristicLength),
		turbu_k_(*particles_->getVariableByName<Real>("TurbulenceKineticEnergy")),
		turbu_epsilon_(*particles_->getVariableByName<Real>("TurbulentDissipation"))
	{
		TurbulentLength_ = 0.07 * CharacteristicLength_ / pow(C_mu, 0.75);
	}
	//=================================================================================================//
	void InflowTurbulentCondition::update(size_t index_i, Real dt)
	{
		Real target_in_turbu_k = getTurbulentInflowK(pos_[index_i], vel_[index_i], turbu_k_[index_i]);
		turbu_k_[index_i] += relaxation_rate_ * (target_in_turbu_k - turbu_k_[index_i]);
		Real target_in_turbu_E = getTurbulentInflowE(pos_[index_i], turbu_k_[index_i], turbu_epsilon_[index_i]);
		turbu_epsilon_[index_i] += relaxation_rate_ * (target_in_turbu_E - turbu_epsilon_[index_i]);
	}
	//=================================================================================================//
	Real InflowTurbulentCondition::getTurbulentInflowK(Vecd& position, Vecd& velocity, Real& turbu_k)
	{
		Real u = velocity[0];
		Real temp_in_turbu_k = 1.5 * pow((TurbulentIntensity * u), 2);
		Real turbu_k_original = turbu_k;
		if (position[0] < 0.0)
		{
			turbu_k_original = temp_in_turbu_k;
		}
		return turbu_k_original;
	}
	//=================================================================================================//
	Real InflowTurbulentCondition::getTurbulentInflowE(Vecd& position, Real& turbu_k, Real& turbu_E)
	{
		//Real temp_in_turbu_E = C_mu * pow(turbu_k, 1.5) / (0.1*getTurbulentLength());
		Real temp_in_turbu_E = pow(turbu_k, 1.5) / TurbulentLength_;
		Real turbu_E_original = turbu_E;
		if (position[0] < 0.0)
		{
			turbu_E_original = temp_in_turbu_E;
		}
		return turbu_E_original;
	}
//=================================================================================================//
	StandardWallFunctionCorrection::
		StandardWallFunctionCorrection(BaseInnerRelation& inner_relation,
			BaseContactRelation& contact_relation, Real offset_dist, const StdVec<int>& id_exclude, NearShapeSurface& near_surface)
		: LocalDynamics(inner_relation.getSPHBody()), FSIContactData(contact_relation), id_exclude_(id_exclude),
		level_set_shape_(&near_surface.level_set_shape_),
		offset_dist_(offset_dist), vel_(particles_->vel_), pos_(particles_->pos_), dimension_(Vecd(0).size()),
		rho_(particles_->rho_), mu_(DynamicCast<Fluid>(this, particles_->getBaseMaterial()).ReferenceViscosity()),
		particle_spacing_(inner_relation.getSPHBody().sph_adaptation_->ReferenceSpacing()),
		cutoff_radius_(inner_relation.getSPHBody().sph_adaptation_->getKernel()->CutOffRadius()),
		turbu_k_(*particles_->getVariableByName<Real>("TurbulenceKineticEnergy")),
		turbu_epsilon_(*particles_->getVariableByName<Real>("TurbulentDissipation")),
		turbu_mu_(*particles_->getVariableByName<Real>("TurbulentViscosity")),
		is_near_wall_P1_(*particles_->getVariableByName<int>("IsNearWallP1")),
		velocity_gradient_(*particles_->getVariableByName<Matd>("VelocityGradient")),
		k_production_(*particles_->getVariableByName<Real>("K_Production"))
	{
		particles_->registerVariable(dist_to_dmy_itfc_ls_, "DistanceToDummyInterfaceLS");
		particles_->registerSortableVariable<Real>("DistanceToDummyInterfaceLS");
		particles_->addVariableToWrite<Real>("DistanceToDummyInterfaceLS");

		particles_->registerVariable(dist_to_dmy_itfc_aver_, "DistanceToDummyInterfaceAver");
		particles_->registerSortableVariable<Real>("DistanceToDummyInterfaceAver");
		particles_->addVariableToWrite<Real>("DistanceToDummyInterfaceAver");

		particles_->registerVariable(y_p_, "Y_P");
		particles_->registerSortableVariable<Real>("Y_P");
		particles_->addVariableToWrite<Real>("Y_P");

		particles_->registerVariable(wall_Y_plus_, "WallYplus");
		particles_->registerSortableVariable<Real>("WallYplus");
		particles_->addVariableToWrite<Real>("WallYplus");

		particles_->registerVariable(wall_Y_star_, "WallYstar");
		particles_->registerSortableVariable<Real>("WallYstar");
		particles_->addVariableToWrite<Real>("WallYstar");

		particles_->registerVariable(is_near_wall_P2_, "IsNearWallP2");
		particles_->registerSortableVariable<int>("IsNearWallP2");
		particles_->addVariableToWrite<int>("IsNearWallP2");
		particles_->registerVariable(is_near_wall_P1_pre_, "IsNearWallP1Pre");
		particles_->registerSortableVariable<int>("IsNearWallP1Pre");
		particles_->addVariableToWrite<int>("IsNearWallP1Pre");
		particles_->registerVariable(is_migrate_, "IsMigrate");
		particles_->registerSortableVariable<int>("IsMigrate");
		particles_->addVariableToWrite<int>("IsMigrate");

		particles_->registerVariable(velo_friction_, "FrictionVelocity");
		particles_->registerSortableVariable<Vecd>("FrictionVelocity");
		particles_->addVariableToWrite<Vecd>("FrictionVelocity");

		particles_->registerVariable(velo_tan_, "TangentialVelocity");
		particles_->registerSortableVariable<Real>("TangentialVelocity");
		particles_->addVariableToWrite<Real>("TangentialVelocity");

		particles_->registerVariable(index_nearest, "NearestIndex");
		particles_->registerSortableVariable<int>("NearestIndex");
		particles_->addVariableToWrite<int>("NearestIndex");

		particles_->registerVariable(dist_to_dmy_interface_, "DistanceToDummyInterface");
		particles_->registerSortableVariable<Real>("DistanceToDummyInterface");
		particles_->addVariableToWrite<Real>("DistanceToDummyInterface");

		/** definition of near wall particles */
		intial_distance_to_wall = 1.5 * particle_spacing_; //changed
		for (size_t k = 0; k != contact_particles_.size(); ++k)
		{
			contact_n_.push_back(&(contact_particles_[k]->n_));
			contact_Vol_.push_back(&(contact_particles_[k]->Vol_));
		}
	};
	//=================================================================================================//
	void StandardWallFunctionCorrection::interaction(size_t index_i, Real dt)
	{
		is_near_wall_P2_[index_i] = 0;
		index_nearest[index_i] = 0;
		velo_tan_[index_i] = 0.0;
		velo_friction_[index_i] = Vecd::Zero();
		wall_Y_plus_[index_i] = 0.0;
		wall_Y_star_[index_i] = 0.0;
		dist_to_dmy_interface_[index_i] = 0.0;
		dist_to_dmy_itfc_ls_[index_i] = 0.0;
		dist_to_dmy_itfc_aver_[index_i] = 0.0;
		is_near_wall_P1_[index_i] = 0;
		y_p_[index_i] = 0.0;
		Real r_dummy_normal = 0.0;
		Real r_dmy_n_j = 0.0;
		Real r_min = 1.0e3;
		Real velo_fric(0.0);
		const Vecd& vel_i = vel_[index_i];
		Real rho_i = rho_[index_i];
		Vecd e_i_nearest_tau = Vecd::Zero();
		Vecd e_n = Vecd::Zero();
		Vecd e_i_nearest_n = Vecd::Zero();
		Matd direc_matrix = Matd::Zero();
		Real ttl_weight(0);
		Real r_dmy_itfc_n_sum(0);
		int count_average(0);

		Matd vel_grad_i_tn = Matd::Zero();  //** velocity gradient of wall-nearest fluid particle i on t-n plane *
		Matd Q = Matd::Zero();

		for (size_t k = 0; k < contact_configuration_.size(); ++k)
		{
			StdLargeVec<Real>& Vol_k = *(contact_Vol_[k]);
			StdLargeVec<Vecd>& n_k = *(contact_n_[k]);
			Neighborhood& contact_neighborhood = (*contact_configuration_[k])[index_i];
			if (contact_neighborhood.current_size_ != 0)
			{
				//r_dummy_normal = abs(level_set_shape_->findSignedDistance(pos_[index_i]));
				dist_to_dmy_itfc_ls_[index_i] = abs(level_set_shape_->findSignedDistance(pos_[index_i]));
				//dist_to_dmy_itfc_ls_[index_i] = r_dummy_normal;
			}
			for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
			{
				size_t index_j = contact_neighborhood.j_[n];
				Real weight_j = contact_neighborhood.W_ij_[n] * Vol_k[index_j];
				Real r_ij = contact_neighborhood.r_ij_[n];
				Vecd& e_ij = contact_neighborhood.e_ij_[n];
				Vecd& n_k_j = n_k[index_j];

				//** The distance to dummy interface is 0.5 dp smaller than the r_ij_normal *  
				r_dmy_n_j = abs(n_k_j.dot(r_ij * e_ij)) - 0.5 * particle_spacing_;

				/*Get the minimum distance, the distance to wall should not be negative*/
				if (r_ij < r_min && r_dmy_n_j > 0.0 + TinyReal)
				{
					r_min = r_ij; //** Find the nearest wall particle *

					//**If use level-set,this would not activate.*
					r_dummy_normal = r_dmy_n_j;

					e_i_nearest_n = n_k[index_j];
					if (dimension_ == 2)
					{
						e_i_nearest_tau[0] = e_i_nearest_n[1];
						e_i_nearest_tau[1] = e_i_nearest_n[0] * (-1.0);
					}
					/*For testing*/
					dist_to_dmy_interface_[index_i] = r_dummy_normal;
					index_nearest[index_i] = index_j;
				}
				/*Double checking*/
				if (r_dummy_normal < 0.0 - TinyReal)
				{
					std::cout << "dist_to_dmy_interface_[index_i] <= 0.0 + TinyReal strange" << std::endl;
					std::cout << r_dmy_n_j << std::endl;
					system("pause");
				}
				//** Only average the bigger value or itself*
				if (r_dmy_n_j - r_dummy_normal > (-1.0 * TinyReal))
				{
					count_average++;
					//** Sum the projection distances according to the kernel approx. *  
					r_dmy_itfc_n_sum += weight_j * r_dmy_n_j;
					ttl_weight += weight_j;
				}
			}
		} //This for statement is to calculate nearest info.

		//** Classify the near wall and sub-near wall paritcles *
		if (r_dummy_normal < (cutoff_radius_ - 0.5 * particle_spacing_) + TinyReal &&
			r_dummy_normal > 0.0 * particle_spacing_ + TinyReal)
		{
			is_near_wall_P2_[index_i] = 10;
			if (r_dummy_normal < 1.0 * particle_spacing_)
			{
				is_near_wall_P1_[index_i] = 1;
			}
		}

		//** Average the distance to wall *
		if (is_near_wall_P2_[index_i] == 10 && ttl_weight != 0.0)
		{
			//** Check the function *
			if (r_dmy_itfc_n_sum <= 0.0)
			{
				std::cout << "r_dmy_itfc_n_sum is almost zero" << std::endl;
				std::cout << "count=" << count_average << std::endl;
				system("pause");
			}
			//** Average the projection distances according to the kernel approx. *  
			r_dummy_normal = r_dmy_itfc_n_sum / ttl_weight;
			dist_to_dmy_itfc_aver_[index_i] = r_dummy_normal;
		}
		//** Check the distance. *  
		if (r_dummy_normal < 0.05 * particle_spacing_ && r_dummy_normal > 0.0)
		{
			std::cout << "There is a particle too close to wall" << std::endl;
			std::cout << "index_i=" << index_i << std::endl;
			std::cout << "DistanceToDummyInterface=" << dist_to_dmy_interface_[index_i] << std::endl;
			std::cout << "DistanceToDummyInterface/dp=" << dist_to_dmy_interface_[index_i] / particle_spacing_ << std::endl;
			std::cout << "DistanceToDummyInterfaceLS=" << dist_to_dmy_itfc_ls_[index_i] << std::endl;
			std::cout << "DistanceToDummyInterfaceLS/dp=" << dist_to_dmy_itfc_ls_[index_i] / particle_spacing_ << std::endl;
			std::cout << "DistanceToDummyInterfaceAver=" << dist_to_dmy_itfc_aver_[index_i] << std::endl;
			std::cout << "DistanceToDummyInterfaceAver/dp=" << dist_to_dmy_itfc_aver_[index_i] / particle_spacing_ << std::endl;
			system("pause");
		}

		//** Calculate friction velocity, including both P region and SUB region. *  
		if (is_near_wall_P2_[index_i] == 10)
		{
			Real velo_tan = 0.0; //** tangitial velo for fluid particle i *
			velo_tan = abs(e_i_nearest_tau.dot(vel_i));
			velo_tan_[index_i] = velo_tan;

			//** Key statement for Offset Model *
			y_p_[index_i] = r_dummy_normal + offset_dist_;

			velo_fric = sqrt(abs(Karman * velo_tan * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) /
				log(turbu_const_E * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) * y_p_[index_i] * rho_i / mu_)));

			if (velo_fric != static_cast<Real>(velo_fric))
			{
				std::cout << "friction velocity is not a real, please check" << std::endl;
				std::cout << "velo_fric=" << velo_fric << "velo_tan=" << velo_tan << std::endl;
				std::cout << "turbu_k_=" << pow(turbu_k_[index_i], 0.5) << std::endl;
				std::cout << "sum=" << (Karman * velo_tan * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) /
					log(turbu_const_E * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) * r_dummy_normal * rho_i / mu_)) << std::endl;
				std::cout << "numerator=" << Karman * velo_tan * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) << std::endl;
				std::cout << "denominator=" << log(turbu_const_E * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) * r_dummy_normal * rho_i / mu_) << std::endl;

				system("pause");
			}
			
			velo_friction_[index_i] = velo_fric * e_i_nearest_tau;
			//** friction velocity have the same direction of vel_i, if not, change its direction *
			if (vel_i.dot(velo_friction_[index_i]) < 0.0)
				velo_friction_[index_i] = -1.0 * velo_friction_[index_i];

			//** Calcualte Y+ *
			wall_Y_plus_[index_i] = y_p_[index_i] * velo_fric * rho_i / mu_;
		}

		//direc_matrix = velo_friction_[index_i].normalized() * e_i_nearest_n.transpose();
		
		// ** Correct the near wall values, only for P region *
		if (is_near_wall_P1_[index_i] == 1) 
		{
			turbu_epsilon_[index_i] = pow(C_mu, 0.75) * pow(turbu_k_[index_i], 1.5) / (Karman * y_p_[index_i]);
			wall_Y_star_[index_i] = y_p_[index_i] * pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) * rho_i / mu_;
			Real denominator = pow(C_mu, 0.25) * pow(turbu_k_[index_i], 0.5) * Karman * y_p_[index_i];
			
			//velocity_gradient_[index_i] = direc_matrix * velo_fric * velo_fric / denominator;

			Real dudn = velo_fric * velo_fric * boost::qvm::sign(vel_i.dot(e_i_nearest_tau)) / denominator;
			vel_grad_i_tn(0, 0) = 0.0;
			vel_grad_i_tn(0, 1) = dudn;
			vel_grad_i_tn(1, 0) = 0.0;
			vel_grad_i_tn(1, 1) = 0.0;
			Q = getTransformationMatrix(e_i_nearest_n);
			velocity_gradient_[index_i] = Q.transpose() * vel_grad_i_tn * Q;

			k_production_[index_i] = rho_i * pow(velo_fric, 4) / denominator;
		}
	}
//=================================================================================================//
//=================================================================================================//
//*********************TESTING MODULES*********************
//=================================================================================================//

//=================================================================================================//
	BaseGetTimeAverageData::BaseGetTimeAverageData(BaseInnerRelation& inner_relation, int num_observer_points)
		: BaseTurtbulentModel<Base, FluidDataInner>(inner_relation),
		pos_(particles_->pos_), num_cell(num_observer_points),
		turbu_k_(*particles_->getVariableByName<Real>("TurbulenceKineticEnergy")),
		turbu_mu_(*particles_->getVariableByName<Real>("TurbulentViscosity")),
		turbu_epsilon_(*particles_->getVariableByName<Real>("TurbulentDissipation")), plt_engine_()
	{
		num_data = 5;
		file_name_.push_back("vel_x_sto_");
		file_name_.push_back("turbu_k_sto_");
		file_name_.push_back("turbu_epsilon_sto_");
		file_name_.push_back("turbu_mu_sto_");
		file_name_.push_back("vel_sto_");

		num_in_cell_.resize(num_cell);
		data_time_aver_sto_.resize(num_cell); //Rows

		data_sto_.resize(num_cell); //Rows
		for (size_t i = 0; i != num_cell; ++i)
		{
			data_sto_[i].resize(num_data); //Cols
		}

		for (size_t j = 0; j != num_data; ++j)
		{
			file_path_output_ = "../bin/output/" + file_name_[j] + ".dat";
			std::ofstream out_file(file_path_output_.c_str(), std::ios::app);
			out_file << "run_time" << "   ";
			for (size_t i = 0; i != num_cell; ++i)
			{
				std::string quantity_name_i = file_name_[j] + "[" + std::to_string(i) + "]";
				plt_engine_.writeAQuantityHeader(out_file, data_sto_[i][j], quantity_name_i);
			}
			out_file << "\n";
			out_file.close();
		}
	}
	//=================================================================================================//
	void BaseGetTimeAverageData::output_time_history_data(Real cutoff_time)
	{
		/** Output for .dat file. */
		for (size_t j = 0; j != num_data; ++j)
		{
			file_path_output_ = "../bin/output/" + file_name_[j] + ".dat";
			std::ofstream out_file(file_path_output_.c_str(), std::ios::app);
			out_file << GlobalStaticVariables::physical_time_ << "   ";
			for (size_t i = 0; i != num_cell; ++i)
			{
				//if (num_in_cell_[i] == 0 && GlobalStaticVariables::physical_time_ > cutoff_time)
				//{
				//	std::cout << "There is a empaty monitoring cell, cell number=" << i << std::endl;
				//	system("pause");
				//}
				num_in_cell_[i] == 0 ? plt_engine_.writeAQuantity(out_file, 0.0) :
					plt_engine_.writeAQuantity(out_file, data_sto_[i][j] / num_in_cell_[i]);
			}
			out_file << "\n";
			out_file.close();
		}
		//** Clear data *
		for (int i = 0; i < num_cell; i++)
		{
			num_in_cell_[i] = 0;
			for (size_t j = 0; j != num_data; ++j)
			{
				data_sto_[i][j] = 0.0;
			}
		}
	}
	//=================================================================================================//
	void BaseGetTimeAverageData::get_time_average_data(Real cutoff_time)
	{
		for (size_t j = 0; j != num_data; ++j)
		{
			data_loaded_.clear();
			int num_line_data = 0;
			//** Load data *
			file_path_input_ = "../bin/output/" + file_name_[j] + ".dat";
			std::ifstream in_file(file_path_input_.c_str());
			bool skipFirstLine = true;
			std::string line;
			while (std::getline(in_file, line))
			{
				if (skipFirstLine)
				{
					skipFirstLine = false;
					continue;
				}
				num_line_data++;
				std::vector<Real> data_point;
				std::istringstream iss(line);
				Real value;
				while (iss >> value)
				{
					data_point.push_back(value);
				}
				data_loaded_.push_back(data_point);
			}

			in_file.close();
			//** Deal with data *
			for (size_t k = 0; k != num_cell; ++k)
			{
				Real sum = 0.0;
				int count = 0;
				for (size_t i = 0; i != num_line_data; ++i)
				{
					if (data_loaded_[i][0] > cutoff_time)
					{
						count++;
						Real delta_t = data_loaded_[i][0] - data_loaded_[i - 1][0];
						sum += data_loaded_[i][k + 1] * delta_t;
						//sum += data_loaded_[i][k + 1]; //**the first col is time*
					}
				}
				//data_time_aver_sto_[k] = sum / count;
				data_time_aver_sto_[k] = sum / (data_loaded_[num_line_data - 1][0] - cutoff_time);
			}
			//** Output data *
			file_path_output_ = "../bin/output/TimeAverageData.dat";
			std::ofstream out_file(file_path_output_.c_str(), std::ios::app);
			out_file << file_name_[j] << "\n";
			for (size_t k = 0; k != num_cell; ++k)
			{
				plt_engine_.writeAQuantity(out_file, data_time_aver_sto_[k]);
			}
			out_file << "\n";
			out_file.close();
		}
		std::cout << "The cutoff_time is " << cutoff_time << std::endl;
	}
	//=================================================================================================//
	GetTimeAverageCrossSectionData::GetTimeAverageCrossSectionData(BaseInnerRelation& inner_relation, int num_observer_points, const StdVec<Real>& bound_x, Real offset_dist_y)
		: BaseGetTimeAverageData(inner_relation, num_observer_points)
	{
		x_min_ = bound_x[0];
		x_max_ = bound_x[1];
		offset_dist_y_ = offset_dist_y;
		//** Get the center coordinate of the monitoring cell *
		for (int i = 0; i < num_cell; i++)
		{
			Real upper_bound = ((i + 1) * particle_spacing_min_ + offset_dist_y_);
			Real lower_bound = (i * particle_spacing_min_ + offset_dist_y_);
			monitor_cellcenter_y.push_back((lower_bound + upper_bound) / 2.0);
		}
		file_path_output_ = "../bin/output/monitor_cell_center_y.dat";
		std::ofstream out_file(file_path_output_.c_str(), std::ios::app);
		for (size_t i = 0; i != num_cell; ++i)
		{
			plt_engine_.writeAQuantity(out_file, monitor_cellcenter_y[i]);
			out_file << "\n";
		}
		out_file << "\n";
		out_file.close();
	}
	//=================================================================================================//
	void GetTimeAverageCrossSectionData::update(size_t index_i, Real dt)
	{
		//** Get data *
		if (pos_[index_i][0] > x_min_ && pos_[index_i][0] <= x_max_)
		{
			for (int i = 0; i < num_cell; i++)
			{
				if (pos_[index_i][1] > (i * particle_spacing_min_ + offset_dist_y_) &&
					pos_[index_i][1] <= ((i + 1) * particle_spacing_min_ + offset_dist_y_))
				{
					num_in_cell_[i] += 1;
					data_sto_[i][0] += vel_[index_i][0];
					data_sto_[i][1] += turbu_k_[index_i];
					data_sto_[i][2] += turbu_epsilon_[index_i];
					data_sto_[i][3] += turbu_mu_[index_i];
					data_sto_[i][4] += vel_[index_i].norm();
				}
			}
		}
	}
	//=================================================================================================//
	GetTimeAverageCenterLineData::GetTimeAverageCenterLineData(BaseInnerRelation& inner_relation,
		int num_observer_points, Real observe_x_ratio, const StdVec<Real>& bound_y, const StdVec<Real>& bound_x_f, const StdVec<Real>& bound_x_b)
		: BaseGetTimeAverageData(inner_relation, num_observer_points), observe_x_ratio_(observe_x_ratio),
		bound_x_f_(bound_x_f), bound_x_b_(bound_x_b), bound_y_(bound_y)
	{
		observe_x_spacing_ = particle_spacing_min_ * observe_x_ratio_;
	}
	//=================================================================================================//
	void GetTimeAverageCenterLineData::update(size_t index_i, Real dt)
	{
		//** Get data *
		if (pos_[index_i][1] > bound_y_[0] && pos_[index_i][1] <= bound_y_[1])
		{
			for (int i = 0; i < num_cell; i++)
			{
				if (i < bound_x_f_.size() - 1) //* Front of cylinder
				{
					if (pos_[index_i][0] > bound_x_f_[i] && pos_[index_i][0] <= bound_x_f_[i + 1])
					{
						num_in_cell_[i] += 1;
						data_sto_[i][0] += vel_[index_i][0];
						data_sto_[i][1] += turbu_k_[index_i];
						data_sto_[i][2] += turbu_epsilon_[index_i];
						data_sto_[i][3] += turbu_mu_[index_i];
						data_sto_[i][4] += vel_[index_i].norm();
					}
				}
				else if (i >= bound_x_f_.size() - 1) //* behind of cylinder
				{
					int j = i - (bound_x_f_.size() - 1);
					if (pos_[index_i][0] > bound_x_b_[j] && pos_[index_i][0] <= bound_x_b_[j + 1])
					{
						num_in_cell_[i] += 1;
						data_sto_[i][0] += vel_[index_i][0];
						data_sto_[i][1] += turbu_k_[index_i];
						data_sto_[i][2] += turbu_epsilon_[index_i];
						data_sto_[i][3] += turbu_mu_[index_i];
						data_sto_[i][4] += vel_[index_i].norm();
					}
				}
			}

		}
	}
	//=================================================================================================//
	void GetTimeAverageCenterLineData::output_monitor_x_coordinate()
	{
		StdVec<Real> monitor_cellcenter_x;
		for (int i = 0; i < bound_x_f_.size() - 1; i++)
		{
			monitor_cellcenter_x.push_back((bound_x_f_[i] + bound_x_f_[i + 1]) / 2.0);
		}
		for (int i = 0; i < bound_x_b_.size() - 1; i++)
		{
			monitor_cellcenter_x.push_back((bound_x_b_[i] + bound_x_b_[i + 1]) / 2.0);
		}

		file_path_output_ = "../bin/output/monitor_cell_center_x.dat";
		std::ofstream out_file(file_path_output_.c_str(), std::ios::app);
		for (size_t i = 0; i != num_cell; ++i)
		{
			plt_engine_.writeAQuantity(out_file, monitor_cellcenter_x[i]);
			out_file << "\n";
		}
		out_file << "\n";
		out_file.close();
	}
//=================================================================================================//
	ClearYPositionForTest::
		ClearYPositionForTest(SPHBody& sph_body)
		: LocalDynamics(sph_body), FluidDataSimple(sph_body),
		pos_(particles_->pos_), vel_(particles_->vel_) {}
	//=================================================================================================//
	void ClearYPositionForTest::update(size_t index_i, Real dt)
	{
		vel_[index_i][1] = 0.0;
	}
//=================================================================================================//
}
//=================================================================================================//
}
//=================================================================================================//