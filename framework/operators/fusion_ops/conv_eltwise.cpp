#include "framework/operators/fusion_ops/conv_eltwise.h"

namespace anakin {

namespace ops {

#define INSTANCE_CONVOLUTION(Ttype, Ptype) \
template<> \
void ConvEltwise<Ttype, Ptype>::operator()(OpContext<Ttype>& ctx, \
    const std::vector<Tensor4dPtr<Ttype> >& ins, \
    std::vector<Tensor4dPtr<Ttype> >& outs) { \
    auto* impl = static_cast<ConvEltwiseHelper<Ttype, Ptype>*>(this->_helper); \
    auto& param = static_cast<ConvEltwiseHelper<Ttype, Ptype>*> \
                  (this->_helper)->_param_conv_eltwise; \
    impl->_funcs_conv_eltwise(ins, outs, param, ctx); \
}

template<typename Ttype, Precision Ptype>
Status ConvEltwiseHelper<Ttype, Ptype>::InitParam() {
    DLOG(WARNING) << "Parsing Conv_eltwise op parameter.";
    saber::ConvParam<Ttype> tmp_conv_param;
    saber::EltwiseParam<Ttype> tmp_eltwise_param;

    // get conv param
    auto group = GET_PARAMETER(int, group);
    auto bias_term = GET_PARAMETER(bool, bias_term);
    auto padding = GET_PARAMETER(PTuple<int>, padding);
    auto strides = GET_PARAMETER(PTuple<int>, strides);
    auto dilation_rate = GET_PARAMETER(PTuple<int>, dilation_rate);
    auto filter_num = GET_PARAMETER(int, filter_num);
    auto kernel_size = GET_PARAMETER(PTuple<int>, kernel_size);
    auto axis = GET_PARAMETER(int, axis);

	using pblock_type = PBlock<Ttype>;
    auto weights = GET_PARAMETER(pblock_type, weight_1);
    auto weights_shape = weights.shape();
    auto weights_dtype = weights.h_tensor().get_dtype();
    // resize weights scale
    auto& w = weights.h_tensor();
    if (w.get_scale().size() == 1){
        float scale_tmp = w.get_scale()[0];
        std::vector<float> w_scale(filter_num, scale_tmp);
        w.set_scale(w_scale);
    }

    // check if this op has batchnorm parameters
    auto has_batchnorm = CHECK_PARAMETER(batchnorm_0_epsilon);
    if (has_batchnorm) { // conv+batchnorm+scale op
        // get batchnorm param
        auto epsilon = GET_PARAMETER(float, batchnorm_0_epsilon);
        auto momentum = GET_PARAMETER(float, batchnorm_0_momentum);
        auto batch_norm_weight_1 = GET_PARAMETER(pblock_type, batchnorm_0_weight_1);
        auto batch_norm_weight_1_vector = batch_norm_weight_1.vector();
        auto batch_norm_weight_2 = GET_PARAMETER(pblock_type, batchnorm_0_weight_2);
        auto batch_norm_weight_2_vector = batch_norm_weight_2.vector();
        auto batch_norm_weight_3 = GET_PARAMETER(pblock_type, batchnorm_0_weight_3);
        auto batch_norm_weight_3_vector = batch_norm_weight_3.vector();

        // get scale param
        auto scale_num_axes = GET_PARAMETER(int, scale_0_num_axes);
        auto scale_bias_term = GET_PARAMETER(bool, scale_0_bias_term);
        auto scale_axis = GET_PARAMETER(int, scale_0_axis);
        auto scale_weight_1 = GET_PARAMETER(pblock_type, scale_0_weight_1);
        auto scale_weight_1_vector = scale_weight_1.vector();
        auto scale_weight_2 = GET_PARAMETER(pblock_type, scale_0_weight_2);
        auto  scale_weight_2_vector = scale_weight_2.vector();

        // check if batchnorm parameters have been optimized
        auto is_param_updated = CHECK_PARAMETER(is_param_updated);
        if (!is_param_updated) {
            SET_PARAMETER(is_param_updated, true, bool);

            if (bias_term) {
                auto bias = GET_PARAMETER(pblock_type, weight_2);
                if (weights_dtype == AK_FLOAT) {
                    graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                            WeightsFusion<float, Ttype>::update_weights, weights, bias,
                            weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                            true, batch_norm_weight_3_vector[0], epsilon,
                            batch_norm_weight_1_vector, batch_norm_weight_2_vector,
                            scale_weight_1_vector, scale_weight_2_vector,
                            scale_bias_term);
                } else {
                    graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                            WeightsFusion<char, Ttype>::update_weights, weights, bias,
                            weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                            true, batch_norm_weight_3_vector[0], epsilon,
                            batch_norm_weight_1_vector, batch_norm_weight_2_vector,
                            scale_weight_1_vector, scale_weight_2_vector,
                            scale_bias_term);
                }

                saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                                                   strides[0], strides[1],
                                                   dilation_rate[0], dilation_rate[1],
                                                   &(weights.d_tensor()), &(bias.d_tensor()));
                tmp_conv_param = conv_param;
            } else {
                pblock_type* bias = new pblock_type();
                SET_PARAMETER(bias_term, true, bool); // set attr bias_term true
                SET_PARAMETER(weight_2, *bias, pblock_type); // gen new bias
                if (weights_dtype == AK_FLOAT) {
                    graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                            WeightsFusion<float, Ttype>::update_weights, weights, *bias,
                            weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                            false, batch_norm_weight_3_vector[0], epsilon,
                            batch_norm_weight_1_vector,
                            batch_norm_weight_2_vector,
                            scale_weight_1_vector,
                            scale_weight_2_vector,
                            scale_bias_term);
                } else {
                    graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                            WeightsFusion<char, Ttype>::update_weights, weights, *bias,
                            weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                            false, batch_norm_weight_3_vector[0], epsilon,
                            batch_norm_weight_1_vector,
                            batch_norm_weight_2_vector,
                            scale_weight_1_vector,
                            scale_weight_2_vector,
                            scale_bias_term);
                }

                saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                        strides[0], strides[1], dilation_rate[0], dilation_rate[1],
                        &(weights.d_tensor()), &(bias->d_tensor()));

                tmp_conv_param = conv_param;
            }
        } else {
            auto bias = GET_PARAMETER(pblock_type, weight_2);
            saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                    strides[0], strides[1], dilation_rate[0], dilation_rate[1],
                    &(weights.d_tensor()), &(bias.d_tensor()));

            tmp_conv_param = conv_param;
        }
    } else { // convolution op
        if (bias_term) {
            auto bias = GET_PARAMETER(pblock_type, weight_2);
            saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                    strides[0], strides[1], dilation_rate[0], dilation_rate[1],
                    &(weights.d_tensor()), &(bias.d_tensor()));
            tmp_conv_param = conv_param;
        } else {
            Tensor4d<Ttype>* bias = new Tensor4d<Ttype>();;
            saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                    strides[0], strides[1], dilation_rate[0], dilation_rate[1],
                    &(weights.d_tensor()), bias);
            tmp_conv_param = conv_param;
        }
    }
    // check eltwise
    auto has_merge_type = CHECK_PARAMETER(merge_type);
    if (has_merge_type) {
        auto type = GET_PARAMETER(std::string, merge_type);
        auto coeff = GET_PARAMETER(PTuple<float>, merge_coeff);

        auto has_alpha = CHECK_PARAMETER(merge_relu_0_alpha);

        EltwiseType elt_type;
        if (type == "Add") {
            elt_type = Eltwise_sum;
        } else if (type == "Max") {
            elt_type = Eltwise_max;
        } else {
                elt_type = Eltwise_prod;
        }
        if (has_alpha) {
            ActivationParam<Ttype> activation_param(Active_relu);
            saber::EltwiseParam<Ttype> eltwise_param(elt_type, coeff.vector(), activation_param);
            tmp_eltwise_param = eltwise_param;
        } else {
            saber::EltwiseParam<Ttype> eltwise_param(elt_type, coeff.vector());
            tmp_eltwise_param = eltwise_param;
        }
        saber::ConvEltwiseParam<Ttype> conv_eltwise_param(tmp_conv_param, tmp_eltwise_param);
        _param_conv_eltwise = conv_eltwise_param;
    } else {
        LOG(FATAL) << "ConvEltwise Op must have been merged eltwise or eltwise + activation.";
    }
    if ((std::is_same<Ttype, NV>::value || std::is_same<Ttype, X86>::value)&& Ptype == Precision::INT8) {
        auto scale_0 = GET_PARAMETER(float, scale_0);
        auto be_eltwise_dtype = GET_PARAMETER(DataType, be_eltwise_dtype);
        float beta = scale_0;
        _param_conv_eltwise.conv_param.beta = beta;
        _param_conv_eltwise.conv_param.beta_type = be_eltwise_dtype;
    }
//    LOG(ERROR) << "framework alpha: "<< _param_conv_eltwise.conv_param.alpha << " beta: " << _param_conv_eltwise.conv_param.beta;
    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status ConvEltwiseHelper<Ttype, Ptype>::Init(OpContext<Ttype>& ctx,
        const std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    auto group = GET_PARAMETER(int, group);
    auto strides = GET_PARAMETER(PTuple<int>, strides);
    auto weights = GET_PARAMETER(PBlock<Ttype>, weight_1);
    auto bias_term = GET_PARAMETER(bool, bias_term);

    //different device pleace change here..
    saber::ImplEnum impl_e = SABER_IMPL;
    // TODO !! output scale is the eltwise_relu output scale!!!
    // THIS IS NOT SUPPORT TO BE THIS WAY, the output scale is not the same with conv_eltwise output scale.
    if ((std::is_same<Ttype, NV>::value||std::is_same<Ttype, X86>::value) && Ptype == Precision::INT8) {
        auto scale_3 = GET_PARAMETER(float, scale_3);
        outs[0]->set_scale({scale_3});
    }

    SABER_CHECK(_funcs_conv_eltwise.init(ins, outs, _param_conv_eltwise, SPECIFY, impl_e, ctx));

    // check if weights have been transposed
    auto is_weights_transed = CHECK_PARAMETER(is_weights_transed);
    if (!is_weights_transed) {
        SET_PARAMETER(is_weights_transed, true, bool);
        if (bias_term) {
            auto bias = GET_PARAMETER(PBlock<Ttype>, weight_2);
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_1>(
                    std::bind(&saber::ConvEltwise<Ttype, PrecisionWrapper<Ptype>::saber_type>::trans_weights,
                              &_funcs_conv_eltwise, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                              weights.d_tensor(), bias.d_tensor(), _param_conv_eltwise.conv_param.pad_h, _param_conv_eltwise.conv_param.pad_w, _param_conv_eltwise.conv_param.dilation_h, _param_conv_eltwise.conv_param.dilation_w,
                              strides[0], strides[1], group, impl_e);
            bias.map_to_host();
        } else {
            PBlock<Ttype> bias_empty;
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_1>(
                    std::bind(&saber::ConvEltwise<Ttype, PrecisionWrapper<Ptype>::saber_type>::trans_weights,
                              &_funcs_conv_eltwise, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                              weights.d_tensor(), bias_empty.d_tensor(), _param_conv_eltwise.conv_param.pad_h, _param_conv_eltwise.conv_param.pad_w, _param_conv_eltwise.conv_param.dilation_h, _param_conv_eltwise.conv_param.dilation_w,
                              strides[0], strides[1], group, impl_e);
        }
        weights.map_to_host();
    } else {
        PBlock<Ttype> weight_empty;
        PBlock<Ttype> bias_empty;
        graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                std::bind(&saber::ConvEltwise<Ttype, PrecisionWrapper<Ptype>::saber_type>::trans_weights,
                        &_funcs_conv_eltwise, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                        weight_empty.d_tensor(), bias_empty.d_tensor(), _param_conv_eltwise.conv_param.pad_h, _param_conv_eltwise.conv_param.pad_w, _param_conv_eltwise.conv_param.dilation_h, _param_conv_eltwise.conv_param.dilation_w,
                        strides[0], strides[1], group, impl_e);
    }
    // TODO beta need some more data to compute!!! this part perhapes will lead some bugs...
    // TODO at least check for scale
    // FIXME don`t add other device for this
    if (std::is_same<Ttype, NV>::value && Ptype == Precision::INT8) {
        float beta = _param_conv_eltwise.conv_param.beta;
        float in_scale = ins[0]->get_scale()[0];
        float weight_scale = _param_conv_eltwise.conv_param.weight()->get_scale()[0];
        beta = beta / in_scale / weight_scale;
//    LOG(ERROR) << " beta = " << beta ;
        _param_conv_eltwise.conv_param.beta = beta;
    }
    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status ConvEltwiseHelper<Ttype, Ptype>::InferShape(const
        std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    SABER_CHECK(_funcs_conv_eltwise.compute_output_shape(ins, outs, _param_conv_eltwise));
    return Status::OK();
}

#ifdef USE_CUDA
template class ConvEltwiseHelper<NV, Precision::FP32>;
template class ConvEltwiseHelper<NV, Precision::FP16>;
template class ConvEltwiseHelper<NV, Precision::INT8>;

INSTANCE_CONVOLUTION(NV, Precision::FP32);
INSTANCE_CONVOLUTION(NV, Precision::INT8);
ANAKIN_REGISTER_OP_HELPER(ConvEltwise, ConvEltwiseHelper, NV, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(ConvEltwise, ConvEltwiseHelper, NV, Precision::INT8);

#endif

#ifdef USE_X86_PLACE
template class ConvEltwiseHelper<X86, Precision::FP32>;
template class ConvEltwiseHelper<X86, Precision::FP16>;
template class ConvEltwiseHelper<X86, Precision::INT8>;

INSTANCE_CONVOLUTION(X86, Precision::FP32);
INSTANCE_CONVOLUTION(X86, Precision::INT8);
ANAKIN_REGISTER_OP_HELPER(ConvEltwise, ConvEltwiseHelper, X86, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(ConvEltwise, ConvEltwiseHelper, X86, Precision::INT8);
#endif

#ifdef USE_ARM_PLACE
INSTANCE_CONVOLUTION(ARM, Precision::FP32);
template class ConvEltwiseHelper<ARM, Precision::FP32>;
ANAKIN_REGISTER_OP_HELPER(ConvEltwise, ConvEltwiseHelper, ARM, Precision::FP32);
#endif

#ifdef AMD_GPU
INSTANCE_CONVOLUTION(AMD, Precision::FP32);
template class ConvEltwiseHelper<AMD, Precision::FP32>;
template class ConvEltwiseHelper<AMD, Precision::FP16>;
template class ConvEltwiseHelper<AMD, Precision::INT8>;
ANAKIN_REGISTER_OP_HELPER(ConvEltwise, ConvEltwiseHelper, AMD, Precision::FP32);
#endif

//! register op
ANAKIN_REGISTER_OP(ConvEltwise)
.Doc("ConvEltwise operator")
#ifdef USE_X86_PLACE
.__alias__<X86, Precision::FP32>("ConvEltwise")
#endif
#ifdef USE_CUDA
.__alias__<NV, Precision::FP32>("ConvEltwise")
.__alias__<NV, Precision::INT8>("ConvEltwise")
#endif
#ifdef AMD_GPU
.__alias__<AMD, Precision::FP32>("ConvEltwise")
#endif
#ifdef USE_ARM_PLACE
.__alias__<ARM, Precision::FP32>("ConvEltwise")
#endif
.num_in(1)
.num_out(1)
.Args<int>("group", " group of conv ")
.Args<bool>("bias_term", " whether conv weights have bias")
.Args<PTuple<int>>("padding", "padding of conv (x, y)")
.Args<PTuple<int>>("strides", "strides of conv (x)")
.Args<PTuple<int>>("dilation_rate", "dilation rate of conv (x)")
.Args<int>("filter_num", "filter(kernel) number of weights")
.Args<PTuple<int>>("kernel_size", "kernel size of kernel (x, y)")
.Args<int>("axis", "axis of conv");

} /* namespace ops */

} /* namespace anakin */


