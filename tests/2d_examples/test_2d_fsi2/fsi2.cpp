/**
 * @file 	fsi2.cpp
 * @brief 	This is the benchmark test of fluid-structure interaction.
 * @details We consider a flow-induced vibration of an elastic beam behind a cylinder in 2D.
 *			The case can be found in Chi Zhang, Massoud Rezavand, Xiangyu Hu,
 *			Dual-criteria time stepping for weakly compressible smoothed particle hydrodynamics.
 *			Journal of Computation Physics 404 (2020) 109135.
 * @author 	Xiangyu Hu, Chi Zhang and Luhui Han
 */
#include "sphinxsys.h"

#include "fsi2.h" //	case file to setup the test case
using namespace SPH;

int main(int ac, char *av[])
{
	//----------------------------------------------------------------------
	//	Build up the environment of a SPHSystem with global controls.
	//----------------------------------------------------------------------
	SPHSystem sph_system(system_domain_bounds, resolution_ref);
	sph_system.setRunParticleRelaxation(false); // Tag for run particle relaxation for body-fitted distribution
	sph_system.setReloadParticles(true);		// Tag for computation with save particles distribution
#ifdef BOOST_AVAILABLE
	sph_system.handleCommandlineOptions(ac, av); // handle command line arguments
#endif
	IOEnvironment io_environment(sph_system);
	//----------------------------------------------------------------------
	//	Creating body, materials and particles.
	//----------------------------------------------------------------------
	FluidBody water_block(sph_system, makeShared<WaterBlock>("WaterBody"));
	water_block.defineParticlesAndMaterial<FluidParticles, WeaklyCompressibleFluid>(rho0_f, c_f, mu_f);
	water_block.generateParticles<ParticleGeneratorLattice>();
	water_block.setParticleSortInterval(100);

	SolidBody wall_boundary(sph_system, makeShared<WallBoundary>("Wall"));
	wall_boundary.defineParticlesAndMaterial<SolidParticles, Solid>();
	wall_boundary.generateParticles<ParticleGeneratorLattice>();

	SolidBody insert_body(sph_system, makeShared<Insert>("InsertedBody"));
	insert_body.defineAdaptationRatios(1.15, 2.0);
	insert_body.defineBodyLevelSetShape()->writeLevelSet(io_environment);
	insert_body.defineParticlesAndMaterial<ElasticSolidParticles, SaintVenantKirchhoffSolid>(rho0_s, Youngs_modulus, poisson);
	(!sph_system.RunParticleRelaxation() && sph_system.ReloadParticles())
		? insert_body.generateParticles<ParticleGeneratorReload>(io_environment, insert_body.getName())
		: insert_body.generateParticles<ParticleGeneratorLattice>();

	ObserverBody beam_observer(sph_system, "BeamObserver");
	beam_observer.generateParticles<ObserverParticleGenerator>(beam_observation_location);
	ObserverBody fluid_observer(sph_system, "FluidObserver");
	fluid_observer.generateParticles<FluidObserverParticleGenerator>();
	//----------------------------------------------------------------------
	//	Run particle relaxation for body-fitted distribution if chosen.
	//----------------------------------------------------------------------
	if (sph_system.RunParticleRelaxation())
	{
		InnerRelation insert_body_inner(insert_body);
		//----------------------------------------------------------------------
		//	Methods used for particle relaxation.
		//----------------------------------------------------------------------
		/** Random reset the insert body particle position. */
		SimpleDynamics<RandomizeParticlePosition> random_insert_body_particles(insert_body);
		/** Write the body state to Vtp file. */
		BodyStatesRecordingToVtp write_insert_body_to_vtp(io_environment, {&insert_body});
		/** Write the particle reload files. */
		ReloadParticleIO write_particle_reload_files(io_environment, {&insert_body});
		/** A  Physics relaxation step. */
		relax_dynamics::RelaxationStepInner relaxation_step_inner(insert_body_inner);
		ReducedQuantityRecording<ReduceAverage<Summation2Norm<Vecd>>>
			insert_body_residue_force_recording(io_environment, insert_body, "Acceleration");
		//----------------------------------------------------------------------
		//	Particle relaxation starts here.
		//----------------------------------------------------------------------
		random_insert_body_particles.parallel_exec(0.25);
		relaxation_step_inner.SurfaceBounding().parallel_exec();
		sph_system.updateSystemCellLinkedLists();
		sph_system.updateSystemRelations();
		//----------------------------------------------------------------------
		//	First output before the main loop.
		//----------------------------------------------------------------------
		write_insert_body_to_vtp.writeToFileByStep();
		//----------------------------------------------------------------------
		//	Relax particles of the insert body.
		//----------------------------------------------------------------------
		while (sph_system.TotalSteps() < 1000)
		{
			relaxation_step_inner.parallel_exec();
			sph_system.accumulateTotalSteps();

			insert_body_residue_force_recording.writeToFileByStep();
			sph_system.monitorSteps("InsertBodyResidueForce", insert_body_residue_force_recording.ResultValue());
			write_insert_body_to_vtp.writeToFileByStep();

			sph_system.updateSystemCellLinkedLists();
			sph_system.updateSystemRelations();
		}
		std::cout << "The physics relaxation process of inserted body finish !" << std::endl;
		write_particle_reload_files.writeToFileByStep();
		return 0;
	}
	//----------------------------------------------------------------------
	//	Define body relation map.
	//	The contact map gives the topological connections between the bodies.
	//	Basically the the range of bodies to build neighbor particle lists.
	//----------------------------------------------------------------------
	TotalLagrangian<InnerRelation> insert_body_inner(insert_body);
	InnerRelation water_block_inner(water_block);
	ContactRelation water_block_contact(water_block, RealBodyVector{&wall_boundary, &insert_body});
	ContactRelation insert_body_contact(insert_body, {&water_block});
	TotalLagrangian<ObservingRelation> beam_observer_contact(beam_observer, RealBodyVector{&insert_body});
	ObservingRelation fluid_observer_contact(fluid_observer, {&water_block});
	//----------------------------------------------------------------------
	//	Combined relations.
	//----------------------------------------------------------------------
	ComplexRelation water_block_complex(water_block_inner, water_block_contact);
	//----------------------------------------------------------------------
	//	Define the main numerical methods used in the simulation.
	//	Note that there may be data dependence on the constructors of these methods.
	//----------------------------------------------------------------------
	/** Initialize particle acceleration. */
	SimpleDynamics<TimeStepInitialization> initialize_a_fluid_step(water_block);
	/** Evaluation of density by summation approach. */
	InteractionWithUpdate<fluid_dynamics::DensitySummationComplex> update_density_by_summation(water_block_complex);
	/** Time step size without considering sound wave speed. */
	ReduceDynamics<fluid_dynamics::AdvectionTimeStepSize> get_fluid_advection_time_step_size(water_block, U_f);
	/** Time step size with considering sound wave speed. */
	ReduceDynamics<fluid_dynamics::AcousticTimeStepSize> get_fluid_time_step_size(water_block);
	/** Pressure relaxation using verlet time stepping. */
	/** Here, we do not use Riemann solver for pressure as the flow is viscous. */
	Dynamics1Level<fluid_dynamics::Integration1stHalfRiemannWithWall> pressure_relaxation(water_block_complex);
	Dynamics1Level<fluid_dynamics::Integration2ndHalfWithWall> density_relaxation(water_block_complex);
	/** viscous acceleration and transport velocity correction can be combined because they are independent dynamics. */
	InteractionDynamics<CombinedLocalInteraction<
		fluid_dynamics::ViscousAccelerationWithWall,
		fluid_dynamics::TransportVelocityCorrectionComplex>>
		viscous_acceleration_and_transport_correction(water_block_complex);
	/** Computing vorticity in the flow. */
	InteractionDynamics<fluid_dynamics::VorticityInner> compute_vorticity(water_block_complex.getInnerRelation());
	/** Inflow boundary condition. */
	BodyAlignedBoxByCell inflow_buffer(
		water_block, makeShared<AlignedBoxShape>(Transform2d(Vec2d(buffer_translation)), buffer_halfsize));
	SimpleDynamics<fluid_dynamics::InflowVelocityCondition<InflowVelocity>, BodyPartByCell> parabolic_inflow(inflow_buffer);
	/** Periodic BCs in x direction. */
	PeriodicConditionUsingCellLinkedList periodic_condition(water_block, water_block.getBodyShapeBounds(), xAxis);
	//----------------------------------------------------------------------
	//	Algorithms of FSI.
	//----------------------------------------------------------------------
	SimpleDynamics<NormalDirectionFromBodyShape> wall_boundary_normal_direction(wall_boundary);
	SimpleDynamics<NormalDirectionFromBodyShape> insert_body_normal_direction(insert_body);
	/** Corrected configuration for the elastic insert body. */
	InteractionDynamics<solid_dynamics::CorrectConfiguration> insert_body_corrected_configuration(insert_body_inner);
	/** Compute the force exerted on solid body due to fluid pressure and viscosity. */
	InteractionDynamics<solid_dynamics::ViscousForceFromFluid> viscous_force_on_solid(insert_body_contact);
	InteractionDynamics<solid_dynamics::AllForceAccelerationFromFluid>
		fluid_force_on_solid_update(insert_body_contact, viscous_force_on_solid);
	/** Compute the average velocity of the insert body. */
	solid_dynamics::AverageVelocityAndAcceleration average_velocity_and_acceleration(insert_body);
	//----------------------------------------------------------------------
	//	Algorithms of solid dynamics.
	//----------------------------------------------------------------------
	/** Compute time step size of elastic solid. */
	ReduceDynamics<solid_dynamics::AcousticTimeStepSize> insert_body_computing_time_step_size(insert_body);
	/** Stress relaxation for the inserted body. */
	Dynamics1Level<solid_dynamics::Integration1stHalf> insert_body_stress_relaxation_first_half(insert_body_inner);
	Dynamics1Level<solid_dynamics::Integration2ndHalf> insert_body_stress_relaxation_second_half(insert_body_inner);
	/** Constrain region of the inserted body. */
	BodyRegionByParticle beam_base(insert_body, makeShared<MultiPolygonShape>(createBeamBaseShape()));
	SimpleDynamics<solid_dynamics::FixConstraint, BodyPartByParticle> constraint_beam_base(beam_base);
	/** Update norm .*/
	SimpleDynamics<solid_dynamics::UpdateElasticNormalDirection> insert_body_update_normal(insert_body);
	//----------------------------------------------------------------------
	//	Define the methods for I/O operations and observations of the simulation.
	//----------------------------------------------------------------------
	BodyStatesRecordingToVtp write_real_body_states(io_environment, sph_system.real_bodies_);
	RegressionTestTimeAveraged<ReducedQuantityRecording<ReduceDynamics<solid_dynamics::TotalForceFromFluid>>>
		write_total_viscous_force_on_insert_body(io_environment, viscous_force_on_solid, "TotalViscousForceOnSolid");
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		write_beam_tip_displacement("Position", io_environment, beam_observer_contact);
	ObservedQuantityRecording<Vecd>
		write_fluid_velocity("Velocity", io_environment, fluid_observer_contact);
	//----------------------------------------------------------------------
	//	Prepare the simulation with cell linked list, configuration
	//	and case specified initial condition if necessary.
	//----------------------------------------------------------------------
	/** initialize cell linked lists for all bodies. */
	sph_system.updateSystemCellLinkedLists();
	/** periodic condition applied after the mesh cell linked list build up
	 * but before the configuration build up. */
	periodic_condition.update_cell_linked_list_.parallel_exec();
	/** initialize configurations for all bodies. */
	sph_system.updateSystemRelations();
	/** computing surface normal direction for the wall. */
	wall_boundary_normal_direction.parallel_exec();
	/** computing surface normal direction for the insert body. */
	insert_body_normal_direction.parallel_exec();
	/** computing linear reproducing configuration for the insert body. */
	insert_body_corrected_configuration.parallel_exec();
	//----------------------------------------------------------------------
	//	Setup computing and initial conditions.
	//----------------------------------------------------------------------
	Real end_time = 200.0;
	Real output_interval = end_time / 200.0;
	//----------------------------------------------------------------------
	//	Statistics for CPU time
	//----------------------------------------------------------------------
	tick_count t1 = tick_count::now();
	tick_count::interval_t interval;
	//----------------------------------------------------------------------
	//	First output before the main loop.
	//----------------------------------------------------------------------
	write_real_body_states.writeToFileByTime();
	write_beam_tip_displacement.writeToFileByStep();
	//----------------------------------------------------------------------
	//	Main loop starts here.
	//----------------------------------------------------------------------
	while (GlobalStaticVariables::physical_time_ < end_time)
	{
		Real integration_time = 0.0;
		/** Integrate time (loop) until the next output time. */
		while (integration_time < output_interval)
		{
			initialize_a_fluid_step.parallel_exec();
			Real Dt = get_fluid_advection_time_step_size.parallel_exec();
			update_density_by_summation.parallel_exec();
			viscous_acceleration_and_transport_correction.parallel_exec();

			/** FSI for viscous force. */
			viscous_force_on_solid.parallel_exec();
			/** Update normal direction on elastic body.*/
			insert_body_update_normal.parallel_exec();
			size_t inner_ite_dt = 0;
			size_t inner_ite_dt_s = 0;
			Real relaxation_time = 0.0;
			Real dt = 0.0;
			while (relaxation_time < Dt)
			{
				dt = SMIN(get_fluid_time_step_size.parallel_exec(), Dt);
				/** Fluid pressure relaxation */
				pressure_relaxation.parallel_exec(dt);
				/** FSI for pressure force. */
				fluid_force_on_solid_update.parallel_exec();
				/** Fluid density relaxation */
				density_relaxation.parallel_exec(dt);

				/** Solid dynamics. */
				inner_ite_dt_s = 0;
				Real dt_s_sum = 0.0;
				average_velocity_and_acceleration.initialize_displacement_.parallel_exec();
				while (dt_s_sum < dt)
				{
					Real dt_s = SMIN(insert_body_computing_time_step_size.parallel_exec(), dt - dt_s_sum);
					insert_body_stress_relaxation_first_half.parallel_exec(dt_s);
					constraint_beam_base.parallel_exec();
					insert_body_stress_relaxation_second_half.parallel_exec(dt_s);
					dt_s_sum += dt_s;
					inner_ite_dt_s++;
				}
				average_velocity_and_acceleration.update_averages_.parallel_exec(dt);

				relaxation_time += dt;
				integration_time += dt;
				GlobalStaticVariables::physical_time_ += dt;
				parabolic_inflow.parallel_exec();
				inner_ite_dt++;
			}
			sph_system.accumulateTotalSteps();

			write_total_viscous_force_on_insert_body.writeToFileByStep();
			write_fluid_velocity.writeToFileByStep();
			write_beam_tip_displacement.writeToFileByStep();
			sph_system.monitorSteps("Time", GlobalStaticVariables::physical_time_,
									"advection_dt", Dt, "acoustic_dt", dt);

			/** Water block configuration and periodic condition. */
			periodic_condition.bounding_.parallel_exec();
			sph_system.updateSystemCellLinkedLists();
			periodic_condition.update_cell_linked_list_.parallel_exec();
			sph_system.updateSystemRelations();
		}

		tick_count t2 = tick_count::now();
		/** write run-time observation into file */
		compute_vorticity.parallel_exec();
		write_real_body_states.writeToFileByTime();
		tick_count t3 = tick_count::now();
		interval += t3 - t2;
	}
	tick_count t4 = tick_count::now();

	tick_count::interval_t tt;
	tt = t4 - t1 - interval;
	std::cout << "Total wall time for computation: " << tt.seconds() << " seconds." << std::endl;

	if (sph_system.generate_regression_data_)
	{
		// The lift force at the cylinder is very small and not important in this case.
		write_total_viscous_force_on_insert_body.generateDataBase({1.0e-2, 1.0e-2}, {1.0e-2, 1.0e-2});
		write_beam_tip_displacement.generateDataBase(1.0e-2);
	}
	else
	{
		write_total_viscous_force_on_insert_body.newResultTest();
		write_beam_tip_displacement.newResultTest();
	}

	return 0;
}
