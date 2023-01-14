#include "base_body.h"

#include "sph_system.h"
#include "base_particles.hpp"
#include "base_body_relation.h"

namespace SPH
{
	//=================================================================================================//
	SPHBody::SPHBody(SPHSystem &sph_system, SharedPtr<Shape> shape_ptr, const std::string &body_name)
		: sph_system_(sph_system), body_name_(body_name),
		  body_shape_(shape_ptr_keeper_.assignPtr(shape_ptr)),
		  sph_adaptation_(sph_adaptation_ptr_keeper_.createPtr<SPHAdaptation>(*this)),
		  base_material_(nullptr), base_particles_(nullptr), newly_updated_(true), newly_moved_(true)
	{
		sph_system_.sph_bodies_.push_back(this);
	}
	//=================================================================================================//
	SPHBody::SPHBody(SPHSystem &sph_system, SharedPtr<Shape> shape_ptr)
		: SPHBody(sph_system, shape_ptr, shape_ptr->getName()) {}
	//=================================================================================================//
	BoundingBox SPHBody::getSPHSystemBounds()
	{
		return sph_system_.system_domain_bounds_;
	}
	//=================================================================================================//
	SPHSystem &SPHBody::getSPHSystem()
	{
		return sph_system_;
	}
	//=================================================================================================//
	void SPHBody::allocateConfigurationMemoriesForBufferParticles()
	{
		for (size_t i = 0; i < all_relations_.size(); i++)
		{
			all_relations_[i]->updateConfigurationMemories();
		}
	}
	//=================================================================================================//
	BoundingBox SPHBody::getBodyShapeBounds()
	{
		return body_shape_->getBounds();
	}
	//=================================================================================================//
	void SPHBody::defineAdaptationRatios(Real h_spacing_ratio, Real new_system_refinement_ratio)
	{
		sph_adaptation_->resetAdaptationRatios(h_spacing_ratio, new_system_refinement_ratio);
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToVtuFile(std::ostream &output_file)
	{
		base_particles_->writeParticlesToVtk(output_file);
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToVtpFile(std::ofstream &output_file)
	{
		base_particles_->writeParticlesToVtk(output_file);
	}
	//=================================================================================================//
	void SPHBody::writeSurfaceParticlesToVtuFile(std::ofstream &output_file, BodySurface &surface_particles)
	{
		base_particles_->writeSurfaceParticlesToVtuFile(output_file, surface_particles);
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToPltFile(std::ofstream &output_file)
	{
		base_particles_->writeParticlesToPltFile(output_file);
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToXmlForRestart(std::string &filefullpath)
	{
		base_particles_->writeParticlesToXmlForRestart(filefullpath);
	}
	//=================================================================================================//
	void SPHBody::readParticlesFromXmlForRestart(std::string &filefullpath)
	{
		base_particles_->readParticleFromXmlForRestart(filefullpath);
	}
	//=================================================================================================//
	void SPHBody::writeToXmlForReloadParticle(std::string &filefullpath)
	{
		base_particles_->writeToXmlForReloadParticle(filefullpath);
	}
	//=================================================================================================//
	void SPHBody::readFromXmlForReloadParticle(std::string &filefullpath)
	{
		base_particles_->readFromXmlForReloadParticle(filefullpath);
	}
	//=================================================================================================//
	BaseCellLinkedList &RealBody::getCellLinkedList()
	{
		if (!cell_linked_list_created_)
		{
			cell_linked_list_ptr_ = std::move(
				sph_adaptation_->createCellLinkedList(getSPHSystemBounds(), *this));
			cell_linked_list_created_ = true;
		}
		return *cell_linked_list_ptr_.get();
	}
	//=================================================================================================//
	void RealBody::updateCellLinkedList()
	{
		if (newly_moved_ && to_update_cell_linked_list_)
		{
			if (iteration_count_ % sorting_interval_ == 0)
			{
				base_particles_->sortParticles(getCellLinkedList());
			}

			iteration_count_++;
			getCellLinkedList().UpdateCellLists(*base_particles_);
			base_particles_->total_ghost_particles_ = 0;

			newly_moved_ = false;
			to_update_cell_linked_list_ = false;
		}
	}
	//=================================================================================================//
	void RealBody::updateCellLinkedListWithParticleSort(size_t particle_sorting_period)
	{
		if (iteration_count_ % particle_sorting_period == 0)
		{
			base_particles_->sortParticles(getCellLinkedList());
		}

		iteration_count_++;
		updateCellLinkedList();
	}
	//=================================================================================================//
}
