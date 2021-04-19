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
 * @file 	base_body.h
 * @brief 	This is the base classes of SPH bodies. The real body is for 
 *			that with cell linked list and the fictitious one does not.     
 * 			Before the definition of the SPH bodies, the shapes with complex 
 *			geometries, i.e. those are produced by advanced binary operation, 
 * 			such as intersection, should be produced first.
 * 			Then, all shapes used in body definition should be either contain 
 * 			or not contain each other. 
 *			Partial overlap between them are not premitted.
 * @author	Luhui Han, Chi ZHang and Xiangyu Hu
 */


#ifndef BASE_BODY_H
#define BASE_BODY_H



#include "base_data_package.h"
#include "sph_data_conainers.h"
#include "particle_adaptation.h"
#include "all_particle_generators.h"
#include "particle_sorting.h"

#include <string>
using namespace std;

namespace SPH 
{
	class SPHSystem;
	class ParticleAdaptation;
	class BaseParticles;
	class BaseMeshCellLinkedList;
	class SPHBodyRelation;
	class ComplexShape;
	class LevelSetComplexShape;

	/**
	 * @class SPHBody
	 * @brief SPHBody is a base body with basic data and functions.
	 *		  Its derived class can be a real fluid body, a real deformable solid body,
	 *        a static or moving solid body or a fictitious body.
	 * 		  Note that only real bodies have cell linked list.
	 */
	class SPHBody
	{
	protected:
		SPHSystem& sph_system_;
		string body_name_;
		bool newly_updated_;		/**< whether this body is in a newly updated state */
		BoundingBox body_domain_bounds_; /**< Computational domain bounds for boundary conditions. */
		bool prescribed_body_bounds_;
	public:
		ParticleAdaptation* particle_adaptation_;	/**< Particle adapation policy. */
		ParticleGenerator* particle_generator_;	/**< Particle generator manner */
		BaseParticles* base_particles_;				/**< Base particles of this body. */
		PositionsAndVolumes body_input_points_volumes_; /**< For direct generate particles. Note this should be moved to direct generator. */
		ComplexShape*  body_shape_;		/** describe the geometry of the body*/
		/**
		 * @brief particle by cells lists is for parallel splitting algorithm.
		 * All particles in each cell are collected together.
		 * If two partiles each belongs two different cell entries,
		 * they have no interaction because they are too far.
		 */
		SplitCellLists split_cell_lists_;

		StdVec<SPHBodyRelation*> body_relations_; /**< all contact relations centered from this body **/

		explicit SPHBody(SPHSystem &sph_system, string body_name, 
			ParticleAdaptation* particle_adaptation = new ParticleAdaptation(),
			ParticleGenerator* particle_generator = new ParticleGeneratorLattice());
		virtual ~SPHBody() {};

		string getBodyName();
		SPHSystem& getSPHSystem();
		void setNewlyUpdated() { newly_updated_ = true; };
		void setNotNewlyUpdated() { newly_updated_ = false; };
		bool checkNewlyUpdated() { return newly_updated_; };

		void setBodyDomainBounds(BoundingBox body_domain_bounds) { body_domain_bounds_ = body_domain_bounds; };
		BoundingBox getBodyDomainBounds() { return body_domain_bounds_; };
		BoundingBox findBodyDomainBounds();
		BoundingBox getSPHSystemBounds();

		/** This will be called in BaseParticle constructor
		 * and is important because particles are not defined in SPHBody constructor.  */
		virtual void assignBaseParticles(BaseParticles* base_particles);
		virtual Real computeReferenceNumberDensity(Vec2d zero = Vec2d(0));
		virtual Real computeReferenceNumberDensity(Vec3d zero = Vec3d(0));
		void allocateConfigurationMemoriesForBufferParticles();

		virtual void writeParticlesToVtuFile(ofstream &output_file);
		virtual void writeParticlesToPltFile(ofstream &output_file);
		virtual void writeParticlesToXmlForRestart(std::string &filefullpath);
		virtual void readParticlesFromXmlForRestart(std::string &filefullpath);
		virtual void writeToXmlForReloadParticle(std::string &filefullpath);
		virtual void readFromXmlForReloadParticle(std::string &filefullpath);
		virtual SPHBody* pointToThisObject() {return this;};
	};
	/**
	 * @class RealBody
	 * @brief Derived class from SPHBody. 
	 * With inner particle configuration or inner interactions.
	 */
	class RealBody : public SPHBody
	{
	public:
		ParticleSorting particle_sorting_;
		BaseMeshCellLinkedList* mesh_cell_linked_list_; /**< Cell linked mesh of this body. */

		RealBody(SPHSystem &sph_system, string body_name, ParticleAdaptation* particle_adaptation,
			ParticleGenerator* particle_generator = new ParticleGeneratorLattice());
		virtual ~RealBody() {};

		/** This will be called in BaseParticle constructor
		 * and is important because particles are not defined in FluidBody constructor.  */
		virtual void assignBaseParticles(BaseParticles* base_particles) override;
		virtual void sortParticleWithMeshCellLinkedList();
		virtual void updateCellLinkedList();
	};

	/**
	 * @class FictitiousBody.
	 * @brief Derived class from SPHBody. 
	 * Without inner configuration or inner interaction.
	 */
	class FictitiousBody : public SPHBody
	{
	public:
		FictitiousBody(SPHSystem& system, string body_name, 
			ParticleAdaptation* particle_adaptation = new  ParticleAdaptation(),
			ParticleGenerator* particle_generator = new ParticleGeneratorDirect());
		virtual ~FictitiousBody() {};
	};

	/**
	 * @class BodyPart
	 * @brief An abstract auxillary class for SPHBody to indicate a part of the body.
	 */
	class BodyPart
	{
	public:
		BodyPart(SPHBody *body, string body_part_name) : 
			body_(body), body_part_name_(body_part_name) {};
		virtual ~BodyPart() {};

		SPHBody* getBody() { return body_; };
		string BodyPartName() { return body_part_name_; };
	protected:
		SPHBody* body_;
		string body_part_name_;

		virtual void tagBodyPart() = 0;
	};

	/**
	 * @class BodyPartByShape
	 * @brief An auxillary class for SPHBody to indicate 
	 * a part of the body defined by a presribed complex shape.
	 */
	class BodyPartByShape : public BodyPart
	{
	public:
		BodyPartByShape(SPHBody* body, string body_part_name);
		virtual ~BodyPartByShape() {};

		ComplexShape* getBodyPartShape() { return body_part_shape_; };
		BoundingBox BodyPartBounds();
	protected:
		ComplexShape* body_part_shape_;
	};
	/**
	 * @class BodyPartByParticle
	 * @brief An auxillary class for SPHBody to 
	 * indicate a part of the body moving together with particles.
	 */
	class BodyPartByParticle : public BodyPartByShape
	{
	public:
		IndexVector body_part_particles_; /**< Collection particle in this body part. */

		BodyPartByParticle(SPHBody* body, string body_part_name)
			: BodyPartByShape(body, body_part_name) {};
		virtual ~BodyPartByParticle() {};
	protected:
		void tagAParticle(size_t particle_index);
		virtual void tagBodyPart() override;
	};

	/**
	 * @class ShapeSurface
	 * @brief A auxillary class for Body to
	 * indicate the surface of a shape
	 */
	class ShapeSurface : public BodyPartByParticle
	{
	public:
		ShapeSurface(SPHBody* body);
		virtual~ShapeSurface() {};

	protected:
		Real particle_spacing_min_;
		virtual void tagBodyPart() override;
	};

	/**
	 * @class ShapeSurfaceLayer
	 * @brief A auxillary class for Body to
	 * indicate the particles within the inner layers of a shape
	 */
	class ShapeSurfaceLayer : public BodyPartByParticle
	{
	public:
		ShapeSurfaceLayer(SPHBody* body, Real layer_thickness = 3.0);
		virtual~ShapeSurfaceLayer() {};

	protected:
		Real thickness_threshold_;

		virtual void tagBodyPart() override;
	};

	/**
	 * @class BodyPartByCell
	 * @brief An auxillary class for SPHBody to
	 * indicate a part of the body fixed in space defined by mesh cells.
	 */
	using namespace std::placeholders;
	class BodyPartByCell : public BodyPartByShape
	{
	protected:
		RealBody* real_body_;
		typedef std::function<bool(Vecd, Real)> CheckIncludedFunctor;
		CheckIncludedFunctor checkIncluded_;

		/** all cells near or contained by the body part shape are included */
		virtual bool checkIncluded(Vecd cell_position, Real threshold);
		virtual void tagBodyPart() override;
	public:
		CellLists body_part_cells_; /**< Collection of cells to indicate the body part. */

		BodyPartByCell(RealBody *real_body, string body_part_name);
		virtual ~BodyPartByCell() {};
	};

	/**
	 * @class NearShapeSurface
	 * @brief An auxillary class for SPHBody to
	 * indicate the region close to the surface of shape.
	 */
	class NearShapeSurface : public BodyPartByCell
	{
	public:
		/** for the case that the body part shape is not that of the body */
		NearShapeSurface(RealBody* real_body, ComplexShape* complex_shape, string body_part_name);
		/** for the case that the body part is the surface of the body shape */
		NearShapeSurface(RealBody* real_body);
		virtual ~NearShapeSurface() {};

		LevelSetComplexShape* getLevelSetComplexShape();
	protected:
		LevelSetComplexShape* level_set_complex_shape_;
		/** only cells near the surface of the body part shape are included */
		virtual bool checkIncluded(Vecd cell_position, Real threshold) override;
	};
}
#endif //BASE_BODY_H