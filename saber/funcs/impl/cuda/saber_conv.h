/* Copyright (c) 2018 Anakin Authors, Inc. All Rights Reserved.

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

#ifndef ANAKIN_SABER_FUNCS_IMPL_CUDA_SABER_CONV_H
#define ANAKIN_SABER_FUNCS_IMPL_CUDA_SABER_CONV_H

#include <vector>
#include "saber/funcs/impl/impl_conv.h"
#include "saber/funcs/funcs_utils.h"

namespace anakin{

namespace saber{

template <DataType OpDtype>
class SaberConv2D<NV, OpDtype> : public ImplBase<
        NV, OpDtype, ConvParam<NV> > {
public:
    typedef typename DataTrait<NV, OpDtype>::Dtype OpDataType;
    typedef ImplBase<NV, OpDtype, ConvParam<NV> > Impl_t;

    SaberConv2D() = default;
    ~SaberConv2D() {
        if (_impl != nullptr) {
            delete _impl;
        }
    }

    SaberStatus init(const std::vector<Tensor<NV> *>& inputs,
            std::vector<Tensor<NV> *>& outputs,
            ConvParam<NV>& param, Context<NV> &ctx) override;

    SaberStatus create(const std::vector<Tensor<NV> *>& inputs,
            std::vector<Tensor<NV> *>& outputs,
            ConvParam<NV>& param, Context<NV>& ctx) override;

    SaberStatus dispatch(const std::vector<Tensor<NV>*>& inputs,
            std::vector<Tensor<NV>*>& outputs,
            ConvParam<NV>& param) override;

    SaberStatus trans_weights(Tensor<NV> &target_weights, Tensor<NV> &target_bias,
                              int pad_h, int pad_w, int dilation_h, int dilation_w,
                              int stride_h, int stride_w, int group);

private:

    std::vector<Tensor<NV> *> _in_data_tensor;
    std::vector<Tensor<NV> *> _out_data_tensor;
    Tensor<NV> int8_input;
    Tensor<NV> int8_output;
    Impl_t* _impl{nullptr};
    bool _extern_trans{false};
    bool _use_vender{false};
    float _in_scale{0.f};
    float _out_scale{0.f};
    bool _scale_per_k{false};
    bool _output_int8{false};

    Tensor<NVHX86> _origin_weight;
    enum KernelAlg{
        K_unknown = 0,
        K_k1s1p0 = 1,
        K_k3s1 = 2,
        K_direct = 3,
        K_depthwise = 4,
        K_vender = 5
    };
    void find_fastest_alg(const std::vector<Tensor<NV>*>& inputs,
                          std::vector<Tensor<NV>*>& outputs,
                          ConvParam<NV>& param, Context<NV> &ctx);
    KernelAlg _kernel_alg{K_unknown};
};
}

}


#endif //ANAKIN_SABER_FUNCS_IMPL_CUDA_SABER_CONV_H
