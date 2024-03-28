#include "general_geometric.h"
#include "base_particles.hpp"

namespace SPH
{
//=============================================================================================//
NormalDirectionFromBodyShape::NormalDirectionFromBodyShape(SPHBody &sph_body)
    : LocalDynamics(sph_body), GeneralDataDelegateSimple(sph_body),
      initial_shape_(*sph_body.initial_shape_), pos_(particles_->pos_),
      n_(*particles_->registerDiscreteVariable<Vecd>("NormalDirection")),
      n0_(*particles_->registerDiscreteVariable<Vecd>("InitialNormalDirection")),
      phi_(*particles_->registerDiscreteVariable<Real>("SignedDistance")),
      phi0_(*particles_->registerDiscreteVariable<Real>("InitialSignedDistance")) {}
//=============================================================================================//
void NormalDirectionFromBodyShape::update(size_t index_i, Real dt)
{
    Vecd normal_direction = initial_shape_.findNormalDirection(pos_[index_i]);
    n_[index_i] = normal_direction;
    n0_[index_i] = normal_direction;
    Real signed_distance = initial_shape_.findSignedDistance(pos_[index_i]);
    phi_[index_i] = signed_distance;
    phi0_[index_i] = signed_distance;
}
//=============================================================================================//
NormalDirectionFromSubShapeAndOp::
    NormalDirectionFromSubShapeAndOp(SPHBody &sph_body, const std::string &shape_name)
    : LocalDynamics(sph_body), GeneralDataDelegateSimple(sph_body),
      shape_and_op_(DynamicCast<ComplexShape>(this, sph_body.initial_shape_)->getSubShapeAndOpByName(shape_name)),
      shape_(shape_and_op_->first),
      switch_sign_(shape_and_op_->second == ShapeBooleanOps::add ? 1.0 : -1.0),
      pos_(particles_->pos_),
      n_(*particles_->registerDiscreteVariable<Vecd>("NormalDirection")),
      n0_(*particles_->registerDiscreteVariable<Vecd>("InitialNormalDirection")),
      phi_(*particles_->registerDiscreteVariable<Real>("SignedDistance")),
      phi0_(*particles_->registerDiscreteVariable<Real>("InitialSignedDistance")) {}
//=============================================================================================//
void NormalDirectionFromSubShapeAndOp::update(size_t index_i, Real dt)
{
    Vecd normal_direction = switch_sign_ * shape_->findNormalDirection(pos_[index_i]);
    n_[index_i] = normal_direction;
    n0_[index_i] = normal_direction;
    Real signed_distance = switch_sign_ * shape_->findSignedDistance(pos_[index_i]);
    phi_[index_i] = signed_distance;
    phi0_[index_i] = signed_distance;
}
//=============================================================================================//
NormalDirectionFromParticles::NormalDirectionFromParticles(BaseInnerRelation &inner_relation)
    : LocalDynamics(inner_relation.getSPHBody()), GeneralDataDelegateInner(inner_relation),
      initial_shape_(*sph_body_.initial_shape_), pos_(particles_->pos_),
      n_(*particles_->registerDiscreteVariable<Vecd>("NormalDirection")),
      n0_(*particles_->registerDiscreteVariable<Vecd>("InitialNormalDirection")),
      phi_(*particles_->registerDiscreteVariable<Real>("SignedDistance")),
      phi0_(*particles_->registerDiscreteVariable<Real>("InitialSignedDistance")) {}
//=================================================================================================//
void NormalDirectionFromParticles::interaction(size_t index_i, Real dt)
{
    Vecd normal_direction = ZeroData<Vecd>::value;
    const Neighborhood &inner_neighborhood = inner_configuration_[index_i];
    for (size_t n = 0; n != inner_neighborhood.current_size_; ++n)
    {
        normal_direction -= inner_neighborhood.dW_ijV_j_[n] * inner_neighborhood.e_ij_[n];
    }
    normal_direction = normal_direction / (normal_direction.norm() + TinyReal);
    n_[index_i] = normal_direction;
    n0_[index_i] = normal_direction;
    Real signed_distance = initial_shape_.findSignedDistance(pos_[index_i]);
    phi_[index_i] = signed_distance;
    phi0_[index_i] = signed_distance;
}
//=================================================================================================//
} // namespace SPH
