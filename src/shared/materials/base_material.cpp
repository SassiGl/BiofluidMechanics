#include "base_material.h"
#include "base_particles.hpp"

namespace SPH
{
	//=================================================================================================//
	void Fluid::registerReloadLocalParameters(BaseParticles* base_particles)
	{
		BaseMaterial::registerReloadLocalParameters(base_particles);
		base_particles->registerVariable(p_, "Pressure");
		base_particles->registerSortableVariable<Real>("Pressure");
		base_particles->addVariableToReload<Real>("Pressure");
		base_particles->registerVariable(drho_dt_, "DensityChangeRate");
		base_particles->registerVariable(surface_indicator_, "SurfaceIndicator");
	}
	//=================================================================================================//
}
