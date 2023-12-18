/* ------------------------------------------------------------------------- *
 *                                SPHinXsys                                  *
 * ------------------------------------------------------------------------- *
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle *
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for    *
 * physical accurate simulation and aims to model coupled industrial dynamic *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH   *
 * (smoothed particle hydrodynamics), a meshless computational method using  *
 * particle discretization.                                                  *
 *                                                                           *
 * SPHinXsys is partially funded by German Research Foundation               *
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,            *
 *  HU1527/12-1 and HU1527/12-4.                                             *
 *                                                                           *
 * Portions copyright (c) 2017-2023 Technical University of Munich and       *
 * the authors' affiliations.                                                *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may   *
 * not use this file except in compliance with the License. You may obtain a *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
/**
 * @file    execution_instance.h
 * @brief   TBD.
 * @author  Alberto Guarnieri and Xiangyu Hu
 */
#ifndef SPHINXSYS_EXECUTION_INSTANCE_H
#define SPHINXSYS_EXECUTION_INSTANCE_H

#include "ownership.h"

#include <sycl/sycl.hpp>

namespace SPH
{
namespace execution
{
class ExecutionInstance
{
  public:
    ExecutionInstance(ExecutionInstance const &) = delete;
    void operator=(ExecutionInstance const &) = delete;

    static ExecutionInstance &getInstance()
    {
        static ExecutionInstance instance;
        return instance;
    }

    sycl::queue &getQueue()
    {
        if (!sycl_queue_)
            sycl_queue_ = makeUnique<sycl::queue>(sycl::gpu_selector_v);
        return *sycl_queue_;
    }

    auto getWorkGroupSize() const
    {
        return work_group_size_;
    }

    void setWorkGroupSize(size_t work_group_size)
    {
        work_group_size_ = work_group_size;
    }

    static inline sycl::nd_range<1> getUniformNdRange(size_t global_size, size_t local_size)
    {
        return {global_size % local_size ? (global_size / local_size + 1) * local_size : global_size, local_size};
    }

    inline sycl::nd_range<1> getUniformNdRange(size_t global_size) const
    {
        // sycl::nd_range is trivially-copyable, no std::move required
        return getUniformNdRange(global_size, work_group_size_);
    }

  private:
    ExecutionInstance() : work_group_size_(32), sycl_queue_() {}

    size_t work_group_size_;
    UniquePtr<sycl::queue> sycl_queue_;

} static &execution_instance = ExecutionInstance::getInstance();
} // namespace execution
} // namespace SPH

#endif // SPHINXSYS_EXECUTION_INSTANCE_H
