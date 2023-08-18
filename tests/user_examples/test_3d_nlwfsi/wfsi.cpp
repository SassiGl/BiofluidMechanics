/**
 * @file 	wfsi.h
 * @brief 	This is the case file for wave impact with tension leg floating structure.
 * @author   Nicolò Salis
 */
#include "sphinxsys.h" //SPHinXsys Library.
using namespace SPH;
#include "wfsi.h" //header for this case
#include "io_simbody_cable.h" //output for cable data

int main()
{
	std::cout << "Mass " << StructureMass << " Volume " << StructureVol <<  " rho_str " << Srho ;
	getchar();
	//----------------------------------------------------------------------
	//	Build up the environment of a SPHSystem with global controls.
	//----------------------------------------------------------------------
	SPHSystem system(system_domain_bounds, particle_spacing_ref);
	IOEnvironment io_environment(system);
	//----------------------------------------------------------------------
	//	Creating body, materials and particles.
	//----------------------------------------------------------------------
	FluidBody water_block(system, makeShared<WaterBlock>("WaterBody"));
	water_block.defineParticlesAndMaterial<FluidParticles, WeaklyCompressibleFluid>(rho0_f, c_f, mu_f);
	water_block.generateParticles<ParticleGeneratorLattice>();

	SolidBody wall_boundary(system, makeShared<WallBoundary>("Wall"));
	wall_boundary.defineParticlesAndMaterial<SolidParticles, Solid>();
	wall_boundary.generateParticles<ParticleGeneratorLattice>();

	SolidBody structure(system, makeShared<FloatingStructure>("Structure"));
	structure.defineParticlesAndMaterial<SolidParticles, Solid>(Srho);
	structure.generateParticles<ParticleGeneratorLattice>();

	ObserverBody observer(system, "Observer");
	observer.defineAdaptationRatios(h, 2.0);
	observer.generateParticles<ObserverParticleGenerator>(
		StdVec<Vecd> {obs});

	ObserverBody WMobserver(system, "WMObserver");
	WMobserver.defineAdaptationRatios(h, 2.0);
	Vecd WMpos0=Vecd(0.0,-Maker_width/2,HWM/2);
	WMobserver.generateParticles<ObserverParticleGenerator>(
		StdVec<Vecd> {WMpos0});	
	//---------------------------------------------------------
	// PRESSURE PROBES
	//---------------------------------------------------------
	ObserverBody tp1(system, "TP1");
	Real tp1x=1.-0.285;
	Real tp1y=12.286+.35;
	Real tp1z=1.043;
	StdVec<Vecd> tp1l={Vecd(tp1x, tp1y, tp1z)};
	tp1.generateParticles<ObserverParticleGenerator>(tp1l);
	ObserverBody tp2(system, "TP2");
	Real tp2x=1.+0.04;
	Real tp2y=12.286+.35;
	Real tp2z=1.043;
	StdVec<Vecd> tp2l={Vecd(tp2x, tp2y, tp2z)};
	tp2.generateParticles<ObserverParticleGenerator>(tp2l);
	ObserverBody fp1(system, "FP1");
	Real fp1x=1.-0.12;
	Real fp1y=12.286;
	Real fp1z=1.013;
	StdVec<Vecd> fp1l={Vecd(fp1x, fp1y, fp1z)};
	fp1.generateParticles<ObserverParticleGenerator>(fp1l);
	ObserverBody fp2(system, "FP2");
	Real fp2x=1.;
	Real fp2y=12.286;
	Real fp2z=0.968;
	StdVec<Vecd> fp2l={Vecd(fp2x, fp2y, fp2z)};
	fp2.generateParticles<ObserverParticleGenerator>(fp2l);
	ObserverBody fp3(system, "FP3");
	Real fp3x=1.;
	Real fp3y=12.286;
	Real fp3z=1.013;
	StdVec<Vecd> fp3l={Vecd(fp3x, fp3y, fp3z)};
	fp3.generateParticles<ObserverParticleGenerator>(fp3l);
	ObserverBody fp4(system, "FP4");
	Real fp4x=1.+0.31;
	Real fp4y=12.286;
	Real fp4z=1.013;
	StdVec<Vecd> fp4l={Vecd(fp4x, fp4y, fp4z)};
	fp4.generateParticles<ObserverParticleGenerator>(fp4l);
	ObserverBody bp1(system, "BP1");
	Real bp1x=1.-0.295;
	Real bp1y=12.286+.35;
	Real bp1z=0.933;
	StdVec<Vecd> bp1l={Vecd(bp1x, bp1y, bp1z)};
	bp1.generateParticles<ObserverParticleGenerator>(bp1l);
	ObserverBody bp2(system, "BP2");
	Real bp2x=1.-0.04;
	Real bp2y=12.286+.35;
	Real bp2z=0.933;
	StdVec<Vecd> bp2l={Vecd(bp2x, bp2y, bp2z)};
	bp2.generateParticles<ObserverParticleGenerator>(bp2l);
	//----------------------------------------------------------------------
	//	Define body relation map.
	//	The contact map gives the topological connections between the bodies.
	//	Basically the the range of bodies to build neighbor particle lists.
	//----------------------------------------------------------------------
	InnerRelation water_block_inner(water_block);
	InnerRelation structure_inner(structure);
	ComplexRelation water_block_complex(water_block_inner, {&wall_boundary, &structure});
	ContactRelation structure_contact(structure, {&water_block});
	ContactRelation observer_contact_with_water(observer, {&water_block});
	ContactRelation observer_contact_with_structure(observer, {&structure});
	ContactRelation WMobserver_contact_with_water(WMobserver, {&water_block});
	ContactRelation WMobserver_contact_with_wall(WMobserver, {&wall_boundary});

	ContactRelation tp1_contact_s(tp1, {&structure});
	ContactRelation tp2_contact_s(tp2, {&structure});
	ContactRelation fp1_contact_s(fp1, {&structure});
	ContactRelation fp2_contact_s(fp2, {&structure});
	ContactRelation fp3_contact_s(fp3, {&structure});
	ContactRelation fp4_contact_s(fp4, {&structure});
	ContactRelation bp1_contact_s(bp1, {&structure});
	ContactRelation bp2_contact_s(bp2, {&structure});
	
	ContactRelation tp1_contact_w(tp1, {&water_block});
	ContactRelation tp2_contact_w(tp2, {&water_block});
	ContactRelation fp1_contact_w(fp1, {&water_block});
	ContactRelation fp2_contact_w(fp2, {&water_block});
	ContactRelation fp3_contact_w(fp3, {&water_block});
	ContactRelation fp4_contact_w(fp4, {&water_block});
	ContactRelation bp1_contact_w(bp1, {&water_block});
	ContactRelation bp2_contact_w(bp2, {&water_block});
	//----------------------------------------------------------------------
	//	Define all numerical methods which are used in this case.
	//----------------------------------------------------------------------
	SimpleDynamics<OffsetInitialPosition> structure_offset_position(structure, offset);
	SimpleDynamics<NormalDirectionFromBodyShape> wall_boundary_normal_direction(wall_boundary);
	SimpleDynamics<NormalDirectionFromBodyShape> structure_normal_direction(structure);
	/** corrected strong configuration. */
	InteractionDynamics<solid_dynamics::CorrectConfiguration> structure_corrected_configuration(structure_inner);
	/** Time step initialization, add gravity. */
	SimpleDynamics<TimeStepInitialization> initialize_time_step_to_fluid(water_block, makeShared<Gravity>(Vecd(0.0, 0.0, -gravity_g)));
	/** Evaluation of density by summation approach. */
	InteractionWithUpdate<fluid_dynamics::DensitySummationFreeSurfaceComplex> update_density_by_summation(water_block_complex);
	/** time step size without considering sound wave speed. */
	ReduceDynamics<fluid_dynamics::AdvectionTimeStepSize> get_fluid_advection_time_step_size(water_block, U_f);
	/** time step size with considering sound wave speed. */
	ReduceDynamics<fluid_dynamics::AcousticTimeStepSize> get_fluid_time_step_size(water_block);
	/** pressure relaxation using Verlet time stepping. */
	Dynamics1Level<fluid_dynamics::Integration1stHalfRiemannWithWall> pressure_relaxation(water_block_complex);
	Dynamics1Level<fluid_dynamics::Integration2ndHalfRiemannWithWall> density_relaxation(water_block_complex);
	/** Computing viscous acceleration. */
	InteractionDynamics<fluid_dynamics::ViscousAccelerationWithWall> viscous_acceleration(water_block_complex);
	/** Damp waves */
	Vecd traslation_damping(0.5*DW,16.5,0.5*HWM);
	Vecd damping(0.5*DW,1.5,0.5*HWM);
	BodyRegionByCell damping_buffer(water_block, makeShared<TransformShape<GeometricShapeBox>>(Transformd(traslation_damping), damping));
	SimpleDynamics<fluid_dynamics::DampingBoundaryCondition> damping_wave(damping_buffer);
	/** Fluid force on structure. */
	InteractionDynamics<solid_dynamics::ViscousForceFromFluid> viscous_force_on_solid(structure_contact);
	InteractionDynamics<solid_dynamics::AllForceAccelerationFromFluid> fluid_force_on_structure(structure_contact, viscous_force_on_solid);
	/** constrain region of the part of wall boundary. */
	BodyRegionByParticle wave_maker(wall_boundary, makeShared<TransformShape<GeometricShapeBox>>(Transformd(translation_wmker), wmker));
	SimpleDynamics<WaveMaking> wave_making(wave_maker);
	//----------------------------------------------------------------------
	//	Define the multi-body system
	//----------------------------------------------------------------------
	std::cout << "Volume " << StructureVol << std::endl;
	std::cout << "MASS " << Srho*StructureVol << std::endl;
	std::cout << "MASS CENTER " << G << std::endl;
	std::cout << "INERTIA " << Ix <<" "<< Iy << " " << Iz << std::endl;
	/** set up the multi body system. */
	SimTK::MultibodySystem MBsystem;
	/** the bodies or matter of the system. */
	SimTK::SimbodyMatterSubsystem matter(MBsystem);
	/** the forces of the system. */
	SimTK::GeneralForceSubsystem forces(MBsystem);
	SimTK::CableTrackerSubsystem cables(MBsystem);
	/** mass properties of the fixed spot. */
	SimTK::Body::Rigid fixed_spot_info(SimTK::MassProperties(1, SimTK::Vec3(0), SimTK::UnitInertia(1)));
	/** mass properties of the structure. */
	StructureSystemForSimbody structure_multibody(structure, makeShared<TriangleMeshShapeSTL>(stl_structure_path, translation_str, StructureScale));
	/** Mass properties of the constrained spot.
	  * SimTK::MassProperties(mass, center of mass, inertia)
	*/
	SimTK::Body::Rigid structure_info(*structure_multibody.body_part_mass_properties_);
	/**
	  * @brief  ** Create a %Planar mobilizer between an existing parent (inboard) body P 
      *	and a new child (outboard) body B created by copying the given \a bodyInfo 
      *	into a privately-owned Body within the constructed %MobilizedBody object. 
      *	Specify the mobilizer frames F fixed to parent P and M fixed to child B. 
	  * @param[in] inboard(SimTK::Vec3) Defines the location of the joint point relative to the parent body.
	  * @param[in] outboard(SimTK::Vec3) Defines the body's origin location to the joint point.
	  * @note	The body's origin location can be the mass center, the the center of mass should be SimTK::Vec3(0)
	  * 			in SimTK::MassProperties(mass, com, inertia)
	  */
	SimTK::MobilizedBody::Planar tethered_strct(matter.Ground(), SimTK::Transform(SimTK::Vec3(translation_str[0],translation_str[1],translation_str[2])), structure_info, SimTK::Transform(SimTK::Vec3(0.0, 0.0, 0.0)));
	/** Mobility of the fixed spot. */
	/*---------------------------------------------------------------------------*/
	SimTK::MobilizedBody::Weld fixed_spotAR( matter.Ground(), SimTK::Transform( 	
						SimTK::Vec3(ground_tethering_AR[0], ground_tethering_AR[1], ground_tethering_AR[2]) ),
						fixed_spot_info, SimTK::Transform(SimTK::Vec3(0.0, 0.0, 0.0)) );
	/*---------------------------------------------------------------------------*/
	SimTK::MobilizedBody::Weld fixed_spotAL( matter.Ground(), SimTK::Transform( 	
						SimTK::Vec3(ground_tethering_AL[0], ground_tethering_AL[1], ground_tethering_AL[2]) ),
						fixed_spot_info, SimTK::Transform(SimTK::Vec3(0.0, 0.0, 0.0)) );
	/*---------------------------------------------------------------------------*/
	SimTK::MobilizedBody::Weld fixed_spotBR( matter.Ground(), SimTK::Transform( 	
						SimTK::Vec3(ground_tethering_BR[0], ground_tethering_BR[1], ground_tethering_BR[2]) ),
						fixed_spot_info, SimTK::Transform(SimTK::Vec3(0.0, 0.0, 0.0)) );
	/*---------------------------------------------------------------------------*/
	SimTK::MobilizedBody::Weld fixed_spotBL( matter.Ground(), SimTK::Transform( 	
						SimTK::Vec3(ground_tethering_BL[0], ground_tethering_BL[1], ground_tethering_BL[2]) ),
						fixed_spot_info, SimTK::Transform(SimTK::Vec3(0.0, 0.0, 0.0)) );
	/*---------------------------------------------------------------------------*/

	//A SEASIDE PILLARS
	//B PORTSIDE PILLARS

	/*-----------------------------------------------------------------------------*/
	Vecd disp_cable_endAR =  structure_tethering_AR - structure_multibody.initial_mass_center_;
	SimTK::CablePath tethering_lineAR(cables, fixed_spotAR, SimTK::Vec3(0.0, 0.0 , 0.0), tethered_strct, SimTK::Vec3(disp_cable_endAR[0], disp_cable_endAR[1], disp_cable_endAR[2]) );
	/*-----------------------------------------------------------------------------*/
	Vecd disp_cable_endAL =  structure_tethering_AL - structure_multibody.initial_mass_center_;
	SimTK::CablePath tethering_lineAL(cables, fixed_spotAL, SimTK::Vec3(0.0, 0.0 , 0.0), tethered_strct, SimTK::Vec3(disp_cable_endAL[0], disp_cable_endAL[1], disp_cable_endAL[2]) );
	/*-----------------------------------------------------------------------------*/
	Vecd disp_cable_endBR =  structure_tethering_BR - structure_multibody.initial_mass_center_;
	SimTK::CablePath tethering_lineBR(cables, fixed_spotBR, SimTK::Vec3(0.0, 0.0 , 0.0), tethered_strct, SimTK::Vec3(disp_cable_endBR[0], disp_cable_endBR[1], disp_cable_endBR[2]) );
	/*-----------------------------------------------------------------------------*/
	Vecd disp_cable_endBL =  structure_tethering_BL - structure_multibody.initial_mass_center_;
	SimTK::CablePath tethering_lineBL(cables, fixed_spotBL, SimTK::Vec3(0.0, 0.0 , 0.0), tethered_strct, SimTK::Vec3(disp_cable_endBL[0], disp_cable_endBL[1], disp_cable_endBL[2]) );

	/*-----------------------------------------------------------------------------*/
		/* CABLE SPRING (forces,
					cable line,
					defaultStiffness, 
						A nonnegative spring constant representing the stiffness of this element, 
						in units of force/length, where the force represents a uniform tension along 
						the element that results from stretching it beyond its slack length.
                    defaultSlackLength,
						The maximum length this elastic element can have before it begins to
						generate force. At or below this length the element is slack and has 
						zero tension and zero power dissipation. 
                    defaultDissipationCoef
						A nonnegative dissipation coefficient for this elastic element in units of 1/velocity.
					);
	*/
	SimTK::CableSpring tethering_springAR(forces, tethering_lineAR, 3.163E5, cablength, 50.);
	SimTK::CableSpring tethering_springAL(forces, tethering_lineAL, 3.163E5, cablength, 50.);
	SimTK::CableSpring tethering_springBR(forces, tethering_lineBR, 3.163E5, cablength, 50.);
	SimTK::CableSpring tethering_springBL(forces, tethering_lineBL, 3.163E5, cablength, 50.);
	SimTK::Force::UniformGravity sim_gravity(forces, matter, SimTK::Vec3(0.0, 0.0,-gravity_g), 0.0);
	/** discrete forces acting on the bodies. */
	SimTK::Force::DiscreteForces force_on_bodies(forces, matter);
	/** Time stepping method for multibody system.*/
	SimTK::State state = MBsystem.realizeTopology();
	SimTK::RungeKuttaMersonIntegrator integ(MBsystem);
	integ.setAccuracy(1e-3);
	integ.setAllowInterpolation(false);
	integ.initialize(state);
	//----------------------------------------------------------------------
	//	Coupling between SimBody and SPH
	//----------------------------------------------------------------------
	ReduceDynamics<solid_dynamics::TotalForceOnBodyPartForSimBody>
	force_on_structure(structure_multibody, MBsystem, tethered_strct, force_on_bodies,integ);
	SimpleDynamics<solid_dynamics::ConstraintBodyPartBySimBody>
	constraint_on_structure(structure_multibody, MBsystem, tethered_strct,force_on_bodies, integ);
	//----------------------------------------------------------------------
	//	Cable SimBody Output
	//----------------------------------------------------------------------
	WriteSimBodyCableData write_cable_AR(io_environment, integ, tethering_springAR,"AR");
	WriteSimBodyCableData write_cable_AL(io_environment, integ, tethering_springAL,"AL");
	WriteSimBodyCableData write_cable_BR(io_environment, integ, tethering_springBR,"BR");
	WriteSimBodyCableData write_cable_BL(io_environment, integ, tethering_springBL,"BL");
	//----------------------------------------------------------------------
	//	Define the methods for I/O operations and observations of the simulation.
	//----------------------------------------------------------------------
	BodyStatesRecordingToVtp write_real_body_states(io_environment, system.real_bodies_);
	/** WaveProbes. */
	BodyRegionByCell wave_probe_buffer(water_block, makeShared<TransformShape<GeometricShapeBox>>(Transformd(translation_WGauge), WGaugeDim));
	ReducedQuantityRecording<ReduceDynamics<FreeSurfaceHeightZ>> wave_gauge(io_environment, wave_probe_buffer);
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_observer_position(observer_contact_with_structure, "Position", "Position");
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		write_str_displacement("Position", io_environment, observer_contact_with_structure);

	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_WMobserver_position(WMobserver_contact_with_wall, "Position", "Position");
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		write_WM_displacement("Position", io_environment, WMobserver_contact_with_wall);

	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_tp1_position(tp1_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_tp2_position(tp2_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_fp1_position(fp1_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_fp2_position(fp2_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_fp3_position(fp3_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_fp4_position(fp4_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_bp1_position(bp1_contact_s, "Position", "Position");
	InteractionDynamics<InterpolatingAQuantity<Vecd>>
		interpolation_bp2_position(bp2_contact_s, "Position", "Position");
	
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		tp1_position_s("Position", io_environment, tp1_contact_s);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		tp2_position_s("Position", io_environment, tp2_contact_s);	
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		fp1_position_s("Position", io_environment, fp1_contact_s);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		fp2_position_s("Position", io_environment, fp2_contact_s);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		fp3_position_s("Position", io_environment, fp3_contact_s);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		fp4_position_s("Position", io_environment, fp4_contact_s);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		bp1_position_s("Position", io_environment, bp1_contact_s);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		bp2_position_s("Position", io_environment, bp2_contact_s);


	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_tp1("Pressure", io_environment, tp1_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_tp2("Pressure", io_environment, tp2_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_fp1("Pressure", io_environment, fp1_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_fp2("Pressure", io_environment, fp2_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_fp3("Pressure", io_environment, fp3_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_fp4("Pressure", io_environment, fp4_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_bp1("Pressure", io_environment, bp1_contact_w);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Real>>
		write_recorded_pressure_bp2("Pressure", io_environment, bp2_contact_w);
	
	//----------------------------------------------------------------------
	//	Basic control parameters for time stepping.
	//----------------------------------------------------------------------
	GlobalStaticVariables::physical_time_ = 0.0;
	int number_of_iterations = 0;
	int screen_output_interval = 1000;
	Real end_time = total_physical_time;
	Real output_interval = end_time/200;
	Real dt = 0.0;
	Real total_time = 0.0;
	Real relax_time = 1.0;
	/** statistics for computing time. */
	TickCount t1 = TickCount::now();
	TimeInterval interval;
	//----------------------------------------------------------------------
	//	Prepare the simulation with cell linked list, configuration
	//	and case specified initial condition if necessary.
	//----------------------------------------------------------------------
	structure_offset_position.exec();
	system.initializeSystemCellLinkedLists();
	system.initializeSystemConfigurations();
	wall_boundary_normal_direction.exec();
	structure_normal_direction.exec();
	structure_corrected_configuration.exec();
	//----------------------------------------------------------------------
	//	First output before the main loop.
	//----------------------------------------------------------------------
	write_real_body_states.writeToFile(number_of_iterations);
	write_str_displacement.writeToFile(number_of_iterations);
	write_WM_displacement.writeToFile(number_of_iterations);
	wave_gauge.writeToFile(number_of_iterations);

	tp1_position_s.writeToFile(number_of_iterations);
	tp2_position_s.writeToFile(number_of_iterations);
	fp1_position_s.writeToFile(number_of_iterations);
	fp2_position_s.writeToFile(number_of_iterations);
	fp3_position_s.writeToFile(number_of_iterations);
	fp4_position_s.writeToFile(number_of_iterations);
	bp1_position_s.writeToFile(number_of_iterations);
	bp2_position_s.writeToFile(number_of_iterations);
				
	write_recorded_pressure_tp1.writeToFile(number_of_iterations);
	write_recorded_pressure_tp2.writeToFile(number_of_iterations);
	write_recorded_pressure_fp1.writeToFile(number_of_iterations);
	write_recorded_pressure_fp2.writeToFile(number_of_iterations);
	write_recorded_pressure_fp3.writeToFile(number_of_iterations);
	write_recorded_pressure_fp4.writeToFile(number_of_iterations);
	write_recorded_pressure_bp1.writeToFile(number_of_iterations);
	write_recorded_pressure_bp2.writeToFile(number_of_iterations);

	write_cable_AR.writeToFile(number_of_iterations);
	write_cable_AL.writeToFile(number_of_iterations);
	write_cable_BR.writeToFile(number_of_iterations);
	write_cable_BL.writeToFile(number_of_iterations);

	// SimBodyStructurePosition.writeToFile(number_of_iterations);
	//----------------------------------------------------------------------
	//	Main loop of time stepping starts here.
	//----------------------------------------------------------------------
	while (GlobalStaticVariables::physical_time_ < end_time)
	{
		Real integral_time = 0.0;
		while (integral_time < output_interval)
		{
			initialize_time_step_to_fluid.exec();

			Real Dt = get_fluid_advection_time_step_size.exec();
			update_density_by_summation.exec();
			viscous_acceleration.exec();
			/** Viscous force exerting on structure. */
			viscous_force_on_solid.exec();

			Real relaxation_time = 0.0;
			while (relaxation_time < Dt)
			{
				dt = get_fluid_time_step_size.exec();

				pressure_relaxation.exec(dt);
				fluid_force_on_structure.exec();
				density_relaxation.exec(dt);
				/** coupled rigid body dynamics. */
				if (total_time >= relax_time)
				{
					 SimTK::State &state_for_update = integ.updAdvancedState();
					 force_on_bodies.clearAllBodyForces(state_for_update);
					 force_on_bodies.setOneBodyForce(state_for_update, tethered_strct, force_on_structure.exec());
					 integ.stepBy(dt);
					 constraint_on_structure.exec();
				     wave_making.exec(dt);
				}
				interpolation_observer_position.exec();
				interpolation_fp2_position.exec();
				interpolation_fp3_position.exec();

				relaxation_time += dt;
				integral_time += dt;
				total_time += dt;
				if (total_time >= relax_time)
					GlobalStaticVariables::physical_time_ += dt;
			}
			

			if (number_of_iterations % screen_output_interval == 0)
			{
				std::cout << std::fixed << std::setprecision(9) << "N=" << number_of_iterations
						  << "	Total Time = " << total_time
						  << "	Physical Time = " << GlobalStaticVariables::physical_time_
						  << "	Dt = " << Dt << "	dt = " << dt << "\n";
			}
			number_of_iterations++;
			damping_wave.exec(Dt);
			water_block.updateCellLinkedListWithParticleSort(100);
			wall_boundary.updateCellLinkedList();
			structure.updateCellLinkedList();
			water_block_complex.updateConfiguration();
			structure_contact.updateConfiguration();
			observer_contact_with_water.updateConfiguration();
			WMobserver_contact_with_water.updateConfiguration();

			tp1_contact_w.updateConfiguration();
			tp2_contact_w.updateConfiguration();
			fp1_contact_w.updateConfiguration();
			fp2_contact_w.updateConfiguration();
			fp3_contact_w.updateConfiguration();
			fp4_contact_w.updateConfiguration();
			bp1_contact_w.updateConfiguration();
			bp2_contact_w.updateConfiguration();
			
			if (total_time >= relax_time)
			{	
				write_str_displacement.writeToFile(number_of_iterations);
				write_WM_displacement.writeToFile(number_of_iterations);
				wave_gauge.writeToFile(number_of_iterations);

				tp1_position_s.writeToFile(number_of_iterations);
				tp2_position_s.writeToFile(number_of_iterations);
				fp1_position_s.writeToFile(number_of_iterations);
				fp2_position_s.writeToFile(number_of_iterations);
				fp3_position_s.writeToFile(number_of_iterations);
				fp4_position_s.writeToFile(number_of_iterations);
				bp1_position_s.writeToFile(number_of_iterations);
				bp2_position_s.writeToFile(number_of_iterations);
				
				write_recorded_pressure_tp1.writeToFile(number_of_iterations);
				write_recorded_pressure_tp2.writeToFile(number_of_iterations);
				write_recorded_pressure_fp1.writeToFile(number_of_iterations);
				write_recorded_pressure_fp2.writeToFile(number_of_iterations);
				write_recorded_pressure_fp3.writeToFile(number_of_iterations);
				write_recorded_pressure_fp4.writeToFile(number_of_iterations);
				write_recorded_pressure_bp1.writeToFile(number_of_iterations);
				write_recorded_pressure_bp2.writeToFile(number_of_iterations);

				write_cable_AR.writeToFile(number_of_iterations);
				write_cable_AL.writeToFile(number_of_iterations);
				write_cable_BR.writeToFile(number_of_iterations);
				write_cable_BL.writeToFile(number_of_iterations);

				// SimBodyStructurePosition.writeToFile(number_of_iterations); *****TODO*****
			}
		}

		TickCount t2 = TickCount::now();
		if (total_time >= relax_time)
			write_real_body_states.writeToFile();
		TickCount t3 = TickCount::now();
		interval += t3 - t2;
	}
	TickCount t4 = TickCount::now();

	TimeInterval tt;
	tt = t4 - t1 - interval;
	std::cout << "Total wall time for computation: " << tt.seconds() << " seconds." << std::endl;

	return 0;
}
