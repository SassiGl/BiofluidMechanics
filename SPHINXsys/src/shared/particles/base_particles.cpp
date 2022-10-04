/**
 * @file base_particles.cpp
 * @brief Definition of functions declared in base_particles.h
 * @author	Xiangyu Hu and Chi Zhang
 */
#include "base_particles.hpp"

#include "base_body.h"
#include "base_body_part.h"
#include "base_material.h"
#include "base_particle_generator.h"
#include "xml_engine.h"

namespace SPH
{
	//=================================================================================================//
	BaseParticles::BaseParticles(SPHBody &sph_body, BaseMaterial *base_material)
		: rho0_(base_material->ReferenceDensity()),
		  sigma0_(sph_body.sph_adaptation_->ReferenceNumberDensity()),
		  speed_max_(0.0), signal_speed_max_(0.0),
		  total_real_particles_(0), real_particles_bound_(0), total_ghost_particles_(0),
		  sph_body_(&sph_body), body_name_(sph_body.getBodyName()),
		  restart_xml_engine_("xml_restart", "particles"),
		  reload_xml_engine_("xml_particle_reload", "particles")
	{
		sph_body.assignBaseParticles(this);
		//----------------------------------------------------------------------
		//		register geometric data only
		//----------------------------------------------------------------------
		registerAVariable(pos_n_, "Position");
		registerAVariable(Vol_, "Volume");
		//----------------------------------------------------------------------
		//		add particle reload data
		//----------------------------------------------------------------------
		addAVariableNameToList<Vecd>(variables_to_reload_, "Position");
		addAVariableNameToList<Real>(variables_to_reload_, "Volume");
	}
	//=================================================================================================//
	void BaseParticles::initializeOtherVariables()
	{
		real_particles_bound_ = total_real_particles_;
		//----------------------------------------------------------------------
		//		register non-geometric data
		//----------------------------------------------------------------------
		registerAVariable(vel_n_, "Velocity");
		registerAVariable(dvel_dt_, "Acceleration");
		registerAVariable(dvel_dt_prior_, "PriorAcceleration");
		registerAVariable(rho_n_, "Density", rho0_);
		registerAVariable(mass_, "Mass");
		//----------------------------------------------------------------------
		//		add basic output particle data
		//----------------------------------------------------------------------
		addAVariableToWrite<Vecd>("Velocity");
		//----------------------------------------------------------------------
		//		add restart output particle data
		//----------------------------------------------------------------------
		addAVariableNameToList<Vecd>(variables_to_restart_, "Position");
		addAVariableNameToList<Vecd>(variables_to_restart_, "Velocity");
		addAVariableNameToList<Vecd>(variables_to_restart_, "Acceleration");
		addAVariableNameToList<Real>(variables_to_restart_, "Volume");
		//----------------------------------------------------------------------
		//		initial particle mass and IDs
		//----------------------------------------------------------------------
		for (size_t i = 0; i != real_particles_bound_; ++i)
		{
			sorted_id_.push_back(sequence_.size());
			sequence_.push_back(0);
			mass_[i] = rho_n_[i] * Vol_[i];
		}
	}
	//=================================================================================================//
	void BaseParticles::addAParticleEntry()
	{
		unsorted_id_.push_back(sequence_.size());
		sorted_id_.push_back(sequence_.size());
		sequence_.push_back(0);

		add_a_particle_value_(all_particle_data_);
	}
	//=================================================================================================//
	void BaseParticles::addBufferParticles(size_t buffer_size)
	{
		for (size_t i = 0; i != buffer_size; ++i)
		{
			addAParticleEntry();
		}
		real_particles_bound_ += buffer_size;
	}
	//=================================================================================================//
	void BaseParticles::copyFromAnotherParticle(size_t this_index, size_t another_index)
	{
		updateFromAnotherParticle(this_index, another_index);
	}
	//=================================================================================================//
	void BaseParticles::updateFromAnotherParticle(size_t this_index, size_t another_index)
	{
		copy_a_particle_value_(all_particle_data_, this_index, another_index);
	}
	//=================================================================================================//
	size_t BaseParticles::insertAGhostParticle(size_t index_i)
	{
		total_ghost_particles_ += 1;
		size_t expected_size = real_particles_bound_ + total_ghost_particles_;
		size_t expected_particle_index = expected_size - 1;
		if (expected_size <= pos_n_.size())
		{
			copyFromAnotherParticle(expected_particle_index, index_i);
			/** For a ghost particle, its sorted id is that of corresponding real particle. */
			sorted_id_[expected_particle_index] = index_i;
		}
		else
		{
			addAParticleEntry();
			copyFromAnotherParticle(expected_particle_index, index_i);
			/** For a ghost particle, its sorted id is that of corresponding real particle. */
			sorted_id_[expected_particle_index] = index_i;
		}
		return expected_particle_index;
	}
	//=================================================================================================//
	void BaseParticles::switchToBufferParticle(size_t index_i)
	{
		size_t last_real_particle_index = total_real_particles_ - 1;
		if (index_i < last_real_particle_index)
		{
			updateFromAnotherParticle(index_i, last_real_particle_index);
			// update unsorted and sorted_id as well
			std::swap(unsorted_id_[index_i], unsorted_id_[last_real_particle_index]);
			sorted_id_[unsorted_id_[index_i]] = index_i;
		}
		total_real_particles_ -= 1;
	}
	//=================================================================================================//
	void BaseParticles::writePltFileHeader(std::ofstream &output_file)
	{
		output_file << " VARIABLES = \"x\",\"y\",\"z\",\"ID\"";

		for (size_t l = 0; l != variables_to_write_[3].size(); ++l)
		{
			std::string variable_name = variables_to_write_[3][l].first;
			output_file << ",\"" << variable_name << "\"";
		};

		for (size_t l = 0; l != variables_to_write_[1].size(); ++l)
		{
			std::string variable_name = variables_to_write_[1][l].first;
			output_file << ",\"" << variable_name << "_x\""
						<< ",\"" << variable_name << "_y\""
						<< ",\"" << variable_name << "_z\"";
		};
		for (size_t l = 0; l != variables_to_write_[0].size(); ++l)
		{
			std::string variable_name = variables_to_write_[0][l].first;
			output_file << ",\"" << variable_name << "\"";
		};
	}
	//=================================================================================================//
	void BaseParticles::writePltFileParticleData(std::ofstream &output_file, size_t index_i)
	{
		// write particle positions and index first
		Vec3d particle_position = upgradeToVector3D(pos_n_[index_i]);
		output_file << particle_position[0] << " " << particle_position[1] << " " << particle_position[2] << " "
					<< index_i << " ";

		for (std::pair<std::string, size_t> &name_index : variables_to_write_[3])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<int> &variable = *(std::get<3>(all_particle_data_)[name_index.second]);
			output_file << variable[index_i] << " ";
		};

		for (std::pair<std::string, size_t> &name_index : variables_to_write_[1])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<Vecd> &variable = *(std::get<1>(all_particle_data_)[name_index.second]);
			Vec3d vector_value = upgradeToVector3D(variable[index_i]);
			output_file << vector_value[0] << " " << vector_value[1] << " " << vector_value[2] << " ";
		};

		for (std::pair<std::string, size_t> &name_index : variables_to_write_[0])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<Real> &variable = *(std::get<0>(all_particle_data_)[name_index.second]);
			output_file << variable[index_i] << " ";
		};
	}
	//=================================================================================================//
	void BaseParticles::writeParticlesToPltFile(std::ofstream &output_file)
	{
		writePltFileHeader(output_file);
		output_file << "\n";

		//compute derived particle variables
		for (ParticleDynamics<void> *derived_variable : derived_variables_)
		{
			derived_variable->parallel_exec();
		}

		size_t total_real_particles = total_real_particles_;
		for (size_t i = 0; i != total_real_particles; ++i)
		{
			writePltFileParticleData(output_file, i);
			output_file << "\n";
		};
	}
	//=================================================================================================//
	void BaseParticles::writeSurfaceParticlesToVtuFile(std::ostream &output_file, BodySurface &surface_particles)
	{
		size_t total_surface_particles = surface_particles.body_part_particles_.size();

		// write current/final particle positions first
		output_file << "   <Points>\n";
		output_file << "    <DataArray Name=\"Position\" type=\"Float32\"  NumberOfComponents=\"3\" Format=\"ascii\">\n";
		output_file << "    ";
		for (size_t i = 0; i != total_surface_particles; ++i)
		{
			size_t particle_i = surface_particles.body_part_particles_[i];
			Vec3d particle_position = upgradeToVector3D(pos_n_[particle_i]);
			output_file << particle_position[0] << " " << particle_position[1] << " " << particle_position[2] << " ";
		}
		output_file << std::endl;
		output_file << "    </DataArray>\n";
		output_file << "   </Points>\n";

		// write header of particles data
		output_file << "   <PointData  Vectors=\"vector\">\n";

		// write sorted particles ID
		// output_file << "    <DataArray Name=\"SortedParticle_ID\" type=\"Int32\" Format=\"ascii\">\n";
		// output_file << "    ";
		// for (size_t i = 0; i != total_surface_particles; ++i)
		// {
		// 	size_t particle_i = surface_particles.body_part_particles_[i];
		// 	output_file << particle_i << " ";
		// }
		// output_file << std::endl;
		// output_file << "    </DataArray>\n";

		// write unsorted particles ID
		// output_file << "    <DataArray Name=\"UnsortedParticle_ID\" type=\"Int32\" Format=\"ascii\">\n";
		// output_file << "    ";
		// for (size_t i = 0; i != total_surface_particles; ++i)
		// {
		// 	size_t particle_i = surface_particles.body_part_particles_[i];
		// 	output_file << unsorted_id_[particle_i] << " ";
		// }
		// output_file << std::endl;
		// output_file << "    </DataArray>\n";

		// write matrices
		for (std::pair<std::string, size_t> &name_index : variables_to_write_[2])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<Matd> &variable = *(std::get<2>(all_particle_data_)[name_index.second]);
			output_file << "    <DataArray Name=\"" << variable_name << "\" type=\"Float32\"  NumberOfComponents=\"9\" Format=\"ascii\">\n";
			output_file << "    ";
			for (size_t i = 0; i != total_surface_particles; ++i)
			{
				size_t particle_i = surface_particles.body_part_particles_[i];
				Mat3d matrix_value = upgradeToMatrix3D(variable[particle_i]);
				for (int k = 0; k != 3; ++k)
				{
					Vec3d col_vector = matrix_value.col(k);
					output_file << std::fixed << std::setprecision(9) << col_vector[0] << " " << col_vector[1] << " " << col_vector[2] << " ";
				}
			}
			output_file << std::endl;
			output_file << "    </DataArray>\n";
		}

		// write vectors
		for (std::pair<std::string, size_t> &name_index : variables_to_write_[1])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<Vecd> &variable = *(std::get<1>(all_particle_data_)[name_index.second]);
			output_file << "    <DataArray Name=\"" << variable_name << "\" type=\"Float32\"  NumberOfComponents=\"3\" Format=\"ascii\">\n";
			output_file << "    ";
			for (size_t i = 0; i != total_surface_particles; ++i)
			{
				size_t particle_i = surface_particles.body_part_particles_[i];
				Vec3d vector_value = upgradeToVector3D(variable[particle_i]);
				output_file << std::fixed << std::setprecision(9) << vector_value[0] << " " << vector_value[1] << " " << vector_value[2] << " ";
			}
			output_file << std::endl;
			output_file << "    </DataArray>\n";
		}

		// write scalars
		for (std::pair<std::string, size_t> &name_index : variables_to_write_[0])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<Real> &variable = *(std::get<0>(all_particle_data_)[name_index.second]);
			output_file << "    <DataArray Name=\"" << variable_name << "\" type=\"Float32\" Format=\"ascii\">\n";
			output_file << "    ";
			for (size_t i = 0; i != total_surface_particles; ++i)
			{
				size_t particle_i = surface_particles.body_part_particles_[i];
				output_file << std::fixed << std::setprecision(9) << variable[particle_i] << " ";
			}
			output_file << std::endl;
			output_file << "    </DataArray>\n";
		}

		// write integers
		for (std::pair<std::string, size_t> &name_index : variables_to_write_[3])
		{
			std::string variable_name = name_index.first;
			StdLargeVec<int> &variable = *(std::get<3>(all_particle_data_)[name_index.second]);
			output_file << "    <DataArray Name=\"" << variable_name << "\" type=\"Int32\" Format=\"ascii\">\n";
			output_file << "    ";
			for (size_t i = 0; i != total_surface_particles; ++i)
			{
				size_t particle_i = surface_particles.body_part_particles_[i];
				output_file << std::fixed << std::setprecision(9) << variable[particle_i] << " ";
			}
			output_file << std::endl;
			output_file << "    </DataArray>\n";
		}
	}
	//=================================================================================================//
	void BaseParticles::resizeXmlDocForParticles(XmlEngine &xml_engine)
	{
		size_t total_elements = xml_engine.SizeOfXmlDoc();

		if (total_elements <= total_real_particles_)
		{
			for (size_t i = total_elements; i != total_real_particles_; ++i)
				xml_engine.addElementToXmlDoc("particle");
		}
	}
	//=================================================================================================//
	void BaseParticles::writeParticlesToXmlForRestart(std::string &filefullpath)
	{
		resizeXmlDocForParticles(restart_xml_engine_);
		WriteAParticleVariableToXml write_variable_to_xml(restart_xml_engine_, total_real_particles_);
		ParticleDataOperation<loopVariableNameList> loop_variable_namelist;
		loop_variable_namelist(all_particle_data_, variables_to_restart_, write_variable_to_xml);
		restart_xml_engine_.writeToXmlFile(filefullpath);
	}
	//=================================================================================================//
	void BaseParticles::readParticleFromXmlForRestart(std::string &filefullpath)
	{
		restart_xml_engine_.loadXmlFile(filefullpath);
		ReadAParticleVariableFromXml read_variable_from_xml(restart_xml_engine_, total_real_particles_);
		ParticleDataOperation<loopVariableNameList> loop_variable_namelist;
		loop_variable_namelist(all_particle_data_, variables_to_restart_, read_variable_from_xml);
	}
	//=================================================================================================//
	void BaseParticles::writeToXmlForReloadParticle(std::string &filefullpath)
	{
		resizeXmlDocForParticles(reload_xml_engine_);
		WriteAParticleVariableToXml write_variable_to_xml(reload_xml_engine_, total_real_particles_);
		ParticleDataOperation<loopVariableNameList> loop_variable_namelist;
		loop_variable_namelist(all_particle_data_, variables_to_reload_, write_variable_to_xml);
		reload_xml_engine_.writeToXmlFile(filefullpath);
	}
	//=================================================================================================//
	void BaseParticles::readFromXmlForReloadParticle(std::string &filefullpath)
	{
		reload_xml_engine_.loadXmlFile(filefullpath);
		total_real_particles_ = reload_xml_engine_.SizeOfXmlDoc();
		for (size_t i = 0; i != total_real_particles_; ++i)
		{
			unsorted_id_.push_back(i);
		};
		resize_particle_data_(all_particle_data_, total_real_particles_);
		ReadAParticleVariableFromXml read_variable_from_xml(reload_xml_engine_, total_real_particles_);
		ParticleDataOperation<loopVariableNameList> loop_variable_namelist;
		loop_variable_namelist(all_particle_data_, variables_to_reload_, read_variable_from_xml);
	}
	//=================================================================================================//
}
