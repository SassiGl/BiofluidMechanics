/* -----------------------------------------------------------------------------*
 *                               SPHinXsys                                      *
 * -----------------------------------------------------------------------------*
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle    *
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for       *
 * physical accurate simulation and aims to model coupled industrial dynamic    *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH      *
 * (smoothed particle hydrodynamics), a meshless computational method using     *
 * particle discretization.                                                     *
 *                                                                              *
 * SPHinXsys is partially funded by German Research Foundation                  *
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,               *
 * HU1527/12-1 and HU1527/12-4	.                                                 *
 *                                                                              *
 * Portions copyright (c) 2017-2022 Technical University of Munich and          *
 * the authors' affiliations.                                                   *
 *                                                                              *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may      *
 * not use this file except in compliance with the License. You may obtain a    *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.           *
 *                                                                              *
 * -----------------------------------------------------------------------------*/
/**
 * @file 	io_observation.h
 * @brief 	Classes for input and output with vtk (Paraview) files.
 * @author	Chi Zhang, Shuoguo Zhang, Zhenxi Zhao and Xiangyu Hu
 */

#pragma once

#include "io_base.h"

namespace SPH
{
	/**
	 * @class ObservedQuantityRecording
	 * @brief write files for observed quantity
	 */
	template <typename VariableType>
	class ObservedQuantityRecording : public BodyStatesRecording,
									  public ObservingAQuantity<VariableType>
	{
	protected:
		SPHBody &observer_;
		PltEngine plt_engine_;
		BaseParticles &base_particles_;
		std::string dynamics_range_name_;
		const std::string quantity_name_;
		size_t observation_step_interval_;
		std::string filefullpath_output_;

	public:
		VariableType type_indicator_; /*< this is an indicator to identify the variable type. */

	public:
		ObservedQuantityRecording(const std::string &quantity_name, IOEnvironment &io_environment,
								  ObservationRelation &observation_relation)
			: BodyStatesRecording(io_environment, observation_relation.sph_body_),
			  ObservingAQuantity<VariableType>(observation_relation, quantity_name),
			  observer_(observation_relation.sph_body_), plt_engine_(),
			  base_particles_(observer_.getBaseParticles()),
			  dynamics_range_name_(observation_relation.sph_body_.getName()),
			  quantity_name_(quantity_name),
			  observation_step_interval_(100)
		{
			/** Output for .dat file. */
			filefullpath_output_ = io_environment_.output_folder_ + "/" + dynamics_range_name_ + "_" + quantity_name + ".dat";
			std::ofstream out_file(filefullpath_output_.c_str(), std::ios::app);
			out_file << "run_time"
					 << "   ";
			for (size_t i = 0; i != base_particles_.total_real_particles_; ++i)
			{
				std::string quantity_name_i = quantity_name + "[" + std::to_string(i) + "]";
				plt_engine_.writeAQuantityHeader(out_file, (*this->interpolated_quantities_)[i], quantity_name_i);
			}
			out_file << "\n";
			out_file.close();
		};
		virtual ~ObservedQuantityRecording(){};

		void setObservationStepInterval(size_t interval) { observation_step_interval_ = interval; };

		virtual void writeToFileByStep() override
		{
			if (sph_system_.TotalSteps() % observation_step_interval_ == 0)
			{
				this->parallel_exec();
				std::ofstream out_file(filefullpath_output_.c_str(), std::ios::app);
				out_file << GlobalStaticVariables::physical_time_ << "   ";
				for (size_t i = 0; i != base_particles_.total_real_particles_; ++i)
				{
					plt_engine_.writeAQuantity(out_file, (*this->interpolated_quantities_)[i]);
				}
				out_file << "\n";
				out_file.close();
			}
		};

		StdLargeVec<VariableType> *getObservedQuantity()
		{
			return this->interpolated_quantities_;
		}
	};

	/**
	 * @class ReducedQuantityRecording
	 * @brief write reduced quantity of a body
	 */
	template <class ReduceMethodType>
	class ReducedQuantityRecording
	{
	protected:
		IOEnvironment &io_environment_;
		PltEngine plt_engine_;
		ReduceMethodType reduce_method_;
		std::string dynamics_range_name_;
		const std::string quantity_name_;
		size_t recording_step_interval_;
		std::string filefullpath_output_;

	public:
		/*< deduce variable type from reduce method. */
		using VariableType = typename ReduceMethodType::ReduceReturnType;
		VariableType type_indicator_; /*< this is an indicator to identify the variable type. */

	public:
		template <typename... ConstructorArgs>
		ReducedQuantityRecording(IOEnvironment &io_environment, ConstructorArgs &&...args)
			: io_environment_(io_environment), plt_engine_(), reduce_method_(std::forward<ConstructorArgs>(args)...),
			  dynamics_range_name_(reduce_method_.DynamicsRangeName()),
			  quantity_name_(reduce_method_.QuantityName()),
			  recording_step_interval_(100)
		{
			/** output for .dat file. */
			filefullpath_output_ = io_environment_.output_folder_ + "/" + dynamics_range_name_ + "_" + quantity_name_ + ".dat";
			std::ofstream out_file(filefullpath_output_.c_str(), std::ios::app);
			out_file << "\"run_time\""
					 << "   ";
			plt_engine_.writeAQuantityHeader(out_file, reduce_method_.Reference(), quantity_name_);
			out_file << "\n";
			out_file.close();
		};
		virtual ~ReducedQuantityRecording(){};

		void setRecordingStepInterval(size_t interval) { recording_step_interval_ = interval; };

		virtual void writeToFileByStep()
		{
			if (sph_system_.TotalSteps() % recording_step_interval_ == 0)
			{
				std::ofstream out_file(filefullpath_output_.c_str(), std::ios::app);
				out_file << GlobalStaticVariables::physical_time_ << "   ";
				plt_engine_.writeAQuantity(out_file, reduce_method_.parallel_exec());
				out_file << "\n";
				out_file.close();
			}
		};
	};
}
