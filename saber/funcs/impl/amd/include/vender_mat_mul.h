/* Copyright (c) 2019 Anakin Authors, Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include "saber/funcs/base.h"
#include "saber/funcs/impl/impl_mat_mul.h"
#include "saber/core/impl/amd/utils/amd_kernel.h"

#include <miopengemm/miogemm.hpp>
#include <miopengemm/gemm.hpp>
#include <miopengemm/geometry.hpp>

#include "include/amd_utils.h"
#include "saber/saber_funcs_param.h"

namespace anakin {

namespace saber {

template <DataType OpDtype>
class VenderMatMul<AMD, OpDtype> : public ImplBase<AMD, OpDtype, MatMulParam<AMD>> {

public:
    typedef typename DataTrait<AMD, OpDtype>::Dtype OpDataType;
    typedef AMD_API::TPtr PtrDtype;

    VenderMatMul() {
        _multikernel      = false;
        _kernels_ptr.clear();
    }

    ~VenderMatMul() {
        _kernels_ptr.clear();
    }

    virtual SaberStatus
    init(const std::vector<Tensor<AMD>*>& inputs,
         std::vector<Tensor<AMD>*>& outputs,
         MatMulParam<AMD>& param,
         Context<AMD>& ctx) override;

    virtual SaberStatus
    create(const std::vector<Tensor<AMD>*>& inputs,
           std::vector<Tensor<AMD>*>& outputs,
           MatMulParam<AMD>& param,
           Context<AMD>& ctx) override;

    virtual SaberStatus dispatch(
        const std::vector<Tensor<AMD>*>& inputs,
        std::vector<Tensor<AMD>*>& outputs,
        MatMulParam<AMD>& param) override;

private:
    std::vector<AMDKernelPtr> _kernels_ptr;
    bool _multikernel = false;
};

} // namespace saber.

} // namespace anakin
