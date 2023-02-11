#include "contact_dynamics.h"

namespace SPH
{
//=============================================================================================================//
	namespace solid_dynamics
	{
		//=====================================================================================================//
		void ShellContactDensity::interaction(size_t index_i, Real dt)
		{
			Real sigma = 0.0; 
			Real contact_density_i = 0.0;

			for (size_t k = 0; k < contact_configuration_.size(); ++k)
			{
				StdLargeVec<Vecd> &contact_n_k = *(contact_n_[k]);
				StdLargeVec<Vecd> &contact_pos_k = *(contact_pos_[k]);
				Neighborhood &contact_neighborhood = (*contact_configuration_[k])[index_i];
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					Vecd contact_pos_j = contact_pos_k[contact_neighborhood.j_[n]];
					Matd transformation_matrix = getTransformationMatrix(contact_n_k[contact_neighborhood.j_[n]]);
					for (int i = 0; i != 3; ++i)
					{
						Vecd x_axis = transformation_matrix * Vec2d{1.0, 0.0};
						Vecd gaussian_points_vector = three_gaussian_points_[i] * x_axis * particle_spacing_ * 0.5;
						Vecd distance_vector = pos_[index_i] - gaussian_points_vector - contact_pos_j;
						Real corrected_W_ij = std::max(kernel_->W(contact_h_ratio_[k], distance_vector.norm(), distance_vector) - offset_W_ij_[k], 0.0);

						sigma += corrected_W_ij * particle_spacing_ * 0.5 * three_gaussian_weights_[i];
					}
				}
				contact_density_i += 0.005 * sigma * calibration_factor_[k];
			}
			contact_density_[index_i] = contact_density_i;
		}
    //=========================================================================================================//
    }
}