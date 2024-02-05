#include "2d_turbulent_half_CD_channel_PBC.h"
using namespace SPH;            


int main(int ac, char *av[])
{
    /**
     * @brief Build up -- a SPHSystem --
     */
    SPHSystem sph_system(system_domain_bounds, resolution_ref);

    /** Tag for run particle relaxation for the initial body fitted distribution. */
    sph_system.setRunParticleRelaxation(false);
    /** Tag for computation start with relaxed body fitted particles distribution. */
    sph_system.setReloadParticles(true);

    sph_system.handleCommandlineOptions(ac, av);
    IOEnvironment io_environment(sph_system);
    /**
     * @brief Material property, particles and body creation of fluid.
     */
    FluidBody water_block(sph_system, makeShared<WaterBlock>("WaterBody"));
    water_block.defineParticlesAndMaterial<BaseParticles, WeaklyCompressibleFluid>(rho0_f, c_f, mu_f);
    water_block.generateParticles<ParticleGeneratorLattice>();

    /**
     * @brief 	Particle and body creation of wall boundary.
     */
    SolidBody wall_boundary(sph_system, makeShared<WallBoundary>("Wall"));
    //wall_boundary.defineAdaptationRatios(1.15, 2.0);
    wall_boundary.defineBodyLevelSetShape();
    wall_boundary.defineParticlesAndMaterial<SolidParticles, Solid>();
    (!sph_system.RunParticleRelaxation() && sph_system.ReloadParticles())
        ? wall_boundary.generateParticles<ParticleGeneratorReload>(io_environment, wall_boundary.getName())
        : wall_boundary.generateParticles<ParticleGeneratorLattice>();
    /** topology */
    InnerRelation water_block_inner(water_block);
    ContactRelation water_wall_contact(water_block, {&wall_boundary});
    //----------------------------------------------------------------------
    // Combined relations built from basic relations
    // which is only used for update configuration.
    //----------------------------------------------------------------------
    ComplexRelation water_block_complex(water_block_inner, water_wall_contact);
    //----------------------------------------------------------------------
    //	Run particle relaxation for body-fitted distribution if chosen.
    //----------------------------------------------------------------------
    if (sph_system.RunParticleRelaxation())
    {
        /** body topology only for particle relaxation */
        InnerRelation wall_boundary_inner(wall_boundary);
        //----------------------------------------------------------------------
        //	Methods used for particle relaxation.
        //----------------------------------------------------------------------
        /** Random reset the insert body particle position. */
        SimpleDynamics<RandomizeParticlePosition> random_inserted_body_particles(wall_boundary);
        /** Write the body state to Vtp file. */
        BodyStatesRecordingToVtp write_inserted_body_to_vtp(io_environment, { &wall_boundary });
        /** Write the particle reload files. */
        ReloadParticleIO write_particle_reload_files(io_environment, wall_boundary);
        /** A  Physics relaxation step. */
        relax_dynamics::RelaxationStepInner relaxation_step_inner(wall_boundary_inner);
        //----------------------------------------------------------------------
        //	Particle relaxation starts here.
        //----------------------------------------------------------------------
        random_inserted_body_particles.exec(0.25);
        relaxation_step_inner.SurfaceBounding().exec();
        write_inserted_body_to_vtp.writeToFile(0);

        int ite_p = 0;
        while (ite_p < 1000)
        {
            relaxation_step_inner.exec();
            ite_p += 1;
            if (ite_p % 200 == 0)
            {
                std::cout << std::fixed << std::setprecision(9) << "Relaxation steps for the inserted body N = " << ite_p << "\n";
                write_inserted_body_to_vtp.writeToFile(ite_p);
            }
        }
        std::cout << "The physics relaxation process of the wall_boundary finish !" << std::endl;

        /** Output results. */
        write_particle_reload_files.writeToFile(0);
        return 0;
    }

    /**
     * @brief 	Define all numerical methods which are used in this case.
    /** Pressure relaxation algorithm with Riemann solver for viscous flows. */
    Dynamics1Level<fluid_dynamics::Integration1stHalfWithWallRiemann> pressure_relaxation(water_block_inner, water_wall_contact);
    /** Density relaxation algorithm by using position verlet time stepping. */
    Dynamics1Level<fluid_dynamics::Integration2ndHalfWithWallNoRiemann> density_relaxation(water_block_inner, water_wall_contact);

    /** Turbulent.Note: When use wall function, K Epsilon calculation only consider inner */
    InteractionWithUpdate<fluid_dynamics::K_TurtbulentModelInner> k_equation_relaxation(water_block_inner, initial_turbu_values);
    InteractionDynamics<fluid_dynamics::GetVelocityGradientInner> get_velocity_gradient(water_block_inner);
    InteractionWithUpdate<fluid_dynamics::E_TurtbulentModelInner> epsilon_equation_relaxation(water_block_inner);
    InteractionDynamics<fluid_dynamics::TKEnergyAccComplex> turbulent_kinetic_energy_acceleration(water_block_inner, water_wall_contact);

    /** Define external force. */
    SimpleDynamics<NormalDirectionFromBodyShape> wall_boundary_normal_direction(wall_boundary);

    /** Turbulent standard wall function needs normal vectors of wall. */
    NearShapeSurface near_surface(water_block, makeShared<WallBoundary>("Wall"));
    near_surface.level_set_shape_.writeLevelSet(io_environment);
    InteractionDynamics<fluid_dynamics::StandardWallFunctionCorrection> standard_wall_function_correction(water_block_inner, water_wall_contact, offset_dist_ref, id_exclude, near_surface);

    SimpleDynamics<fluid_dynamics::GetTimeAverageCrossSectionData, SequencedPolicy> get_time_average_cross_section_data(water_block_inner, num_observer_points, monitoring_bound);

    /** Choose one, ordinary or turbulent. Computing viscous acceleration, */
    InteractionDynamics<fluid_dynamics::TurbulentViscousAccelerationWithWall> turbulent_viscous_acceleration(water_block_inner, water_wall_contact);
    //InteractionDynamics<fluid_dynamics::ViscousAccelerationWithWall> viscous_acceleration(water_block_inner, water_wall_contact);
    
    /** Impose transport velocity. */
    InteractionWithUpdate<fluid_dynamics::TransportVelocityCorrectionComplex<AllParticles>> transport_velocity_correction(water_block_inner, water_wall_contact);
    /** Evaluation of density by summation approach. */
    InteractionWithUpdate<fluid_dynamics::DensitySummationComplex> update_density_by_summation(water_block_inner, water_wall_contact);
    water_block.addBodyStateForRecording<Real>("Pressure");		   // output for debug
    water_block.addBodyStateForRecording<int>("Indicator"); // output for debug
    water_block.addBodyStateForRecording<Real>("Density"); // output for debug

    /** Initialize particle acceleration. */
    SimpleDynamics<TimeStepInitialization> initialize_a_fluid_step(water_block, makeShared<Gravity>(Vecd(gravity_g, 0.0)));
    /** Periodic BCs in x direction. */
    PeriodicConditionUsingCellLinkedList periodic_condition(water_block, water_block.getBodyShapeBounds(), xAxis);


    /** Choose one, ordinary or turbulent. Time step size without considering sound wave speed. */
    ReduceDynamics<fluid_dynamics::TurbulentAdvectionTimeStepSize> get_turbulent_fluid_advection_time_step_size(water_block, U_f);
    //ReduceDynamics<fluid_dynamics::AdvectionTimeStepSize> get_fluid_advection_time_step_size(water_block, U_f);
    
    /** Time step size with considering sound wave speed. */
    ReduceDynamics<fluid_dynamics::AcousticTimeStepSize> get_fluid_time_step_size(water_block);

    /** Turbulent eddy viscosity calculation needs values of Wall Y start. */
    SimpleDynamics<fluid_dynamics::TurbulentEddyViscosity> update_eddy_viscosity(water_block);

    //SimpleDynamics<fluid_dynamics::ClearYPositionForTest> clear_y_displacement(water_block);


    /** Output the body states. */
    BodyStatesRecordingToVtp body_states_recording(io_environment, sph_system.real_bodies_);

    /**
     * @brief Setup geometry and initial conditions.
     */
    sph_system.initializeSystemCellLinkedLists();
    periodic_condition.update_cell_linked_list_.exec();
    sph_system.initializeSystemConfigurations();
    wall_boundary_normal_direction.exec();
    wall_boundary.addBodyStateForRecording<Vecd>("NormalDirection");
    /** Output the start states of bodies. */
    body_states_recording.writeToFile(0);
    //----------------------------------------------------------------------
    //	Setup computing and initial conditions.
    //----------------------------------------------------------------------
    size_t number_of_iterations = sph_system.RestartStep();
    int screen_output_interval = 100;
    Real end_time = 600.0;   /**< End time. */
    Real Output_Time = end_time / 40.0; /**< Time stamps for output of body states. */
    Real dt = 0.0;          /**< Default acoustic time step sizes. */
    //----------------------------------------------------------------------
    //	Statistics for CPU time
    //----------------------------------------------------------------------
    TickCount t1 = TickCount::now();
    TimeInterval interval;
    //----------------------------------------------------------------------------------------------------
    //	Main loop starts here.
    //----------------------------------------------------------------------------------------------------
    int num_output_file = 0;
    while (GlobalStaticVariables::physical_time_ < end_time)
    {
        Real integration_time = 0.0;
        /** Integrate time (loop) until the next output time. */
        while (integration_time < Output_Time)
        {
            /** Acceleration due to viscous force and gravity. */
            initialize_a_fluid_step.exec();
            
            //Real Dt = get_fluid_advection_time_step_size.exec();
            Real Dt = get_turbulent_fluid_advection_time_step_size.exec();

            update_density_by_summation.exec();

            update_eddy_viscosity.exec();

            //viscous_acceleration.exec();
            turbulent_viscous_acceleration.exec();

            transport_velocity_correction.exec();
            /** Dynamics including pressure relaxation. */
            Real relaxation_time = 0.0;
            int inner_itr = 0;
            while (relaxation_time < Dt)
            {
                //std::cout << "inner_itr=" << inner_itr << std::endl;

                dt = SMIN(get_fluid_time_step_size.exec(), Dt);
                
                turbulent_kinetic_energy_acceleration.exec();

                pressure_relaxation.exec(dt);

                density_relaxation.exec(dt);
                
                get_velocity_gradient.exec(dt);
                k_equation_relaxation.exec(dt);
                epsilon_equation_relaxation.exec(dt);
                standard_wall_function_correction.exec();

                relaxation_time += dt;
                integration_time += dt;
                GlobalStaticVariables::physical_time_ += dt;
                inner_itr++;

                //clear_y_displacement.exec();
                
            }
            if (number_of_iterations % screen_output_interval == 0)
            {
                std::cout << std::fixed << std::setprecision(9) << "N=" << number_of_iterations << "	Time = "
                          << GlobalStaticVariables::physical_time_
                          << "	Dt = " << Dt << "	dt = " << dt << "\n";
            }
            number_of_iterations++;
            /** Update cell linked list and configuration. */
            /** Water block configuration and periodic condition. */
            periodic_condition.bounding_.exec();
            water_block.updateCellLinkedListWithParticleSort(100);
            periodic_condition.update_cell_linked_list_.exec();
            water_block_complex.updateConfiguration();

            get_time_average_cross_section_data.exec();
            get_time_average_cross_section_data.output_time_history_data(end_time * 0.75);

        }
        TickCount t2 = TickCount::now();
        
        body_states_recording.writeToFile();
        num_output_file++;
        if (num_output_file == 20)
            //system("pause");
        
        TickCount t3 = TickCount::now();
    }
    TickCount t4 = TickCount::now();

    TimeInterval tt;
    tt = t4 - t1 - interval;
    std::cout << "Total wall time for computation: " << tt.seconds()
              << " seconds." << std::endl;

    get_time_average_cross_section_data.get_time_average_data(end_time * 0.75);
    std::cout << "The time-average data is output " << std::endl;

    return 0;
}
