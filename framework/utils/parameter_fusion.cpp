#include "framework/utils/parameter_fusion.h"
namespace anakin {
/**
 * \brief  update fp32 conv weights with batchnorm and scale parameters.
 */
template<typename T>
void WeightsFusion<float, T>::update_weights(
                    PBlock<T> weights, PBlock<T> bias,
                    int n, int c, int h, int w, bool conv_bias_term,
                    float batchnorm_scale, float batchnorm_eps,
                    std::vector<float> batchnorm_mean,
                    std::vector<float> batchnorm_variance,
                    std::vector<float> scale_w,
                    std::vector<float> scale_b,
                    bool scale_bias_term) {
    float* weights_p = (float*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;

        // insert scale parameters
        alpha = scale_w[i] * alpha;
        if (scale_bias_term) {
            beta = beta * scale_w[i] + scale_b[i];
        } else {
            beta = beta * scale_w[i];
        }
        int start_index = i * chw;
        for (int j = 0; j < chw; j++) {
            weights_p[start_index + j] *= alpha;
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update fp32 conv weights with affine channel parameters.
 */
template<typename T>
void WeightsFusion<float, T>::update_conv_affine_channel_weights(
                    PBlock<T> weights, PBlock<T> bias,
                    int n, int c, int h, int w,
                    std::vector<float> affine_channel_w,
                    std::vector<float> affine_channel_b) {
    float* weights_p = (float*)(weights.h_tensor().mutable_data());
    float* bias_p = (float* )(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < chw; j++) {
            weights_p[i * chw + j] *= affine_channel_w[i];
        }
        bias_p[i] = bias_p[i] * affine_channel_w[i] + affine_channel_b[i];
    }
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update fp32 conv weights with batchnorm.
 */
template<typename T>
void WeightsFusion<float, T>::update_weights_without_scale(
                    PBlock<T> weights, PBlock<T> bias,
                    int n, int c, int h, int w, bool conv_bias_term,
                    float batchnorm_scale, float batchnorm_eps,
                    std::vector<float> batchnorm_mean,
                    std::vector<float> batchnorm_variance) {
    float* weights_p = (float* )(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();

    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;
        int start_index = i * chw;
        for (int j = 0; j < chw; j++) {
            weights_p[start_index + j] *= alpha;
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

template<typename T>
void WeightsFusion<float, T>::update_weights_conv_scale(PBlock<T> weights, PBlock<T> bias,
                               int n, int c, int h, int w, bool conv_bias_term,
                               std::vector<float> scale_w,
                               std::vector<float> scale_b,
                               bool scale_bias_term){
    float* weights_p = (float*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, scale_w.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();

    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        float alpha = scale_w[i];
        float beta = 0.f;
        if (scale_bias_term) {
            beta = scale_b[i];
        }
        int start_index = i * chw;
        for (int j = 0; j < chw; j++) {
            weights_p[start_index + j] *= alpha;
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update fp32 deconv weights with batchnorm and scale parameters.
 */
template<typename T>
void WeightsFusion<float, T>::update_deconv_weights(
                    PBlock<T> weights, PBlock<T> bias,
                    int n, int c, int h, int w, bool conv_bias_term,
                    float batchnorm_scale, float batchnorm_eps,
                    std::vector<float> batchnorm_mean,
                    std::vector<float> batchnorm_variance,
                    std::vector<float> scale_w,
                    std::vector<float> scale_b,
                    bool scale_bias_term) {
    float* weights_p = (float*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();

    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    //swap n and c
    int tn = c;
    c = n;
    n = tn;

    int chw = c * h * w;
    int hw = h * w;
    for (int i = 0; i < c; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;

        // insert scale parameters
        alpha = scale_w[i] * alpha;
        if (scale_bias_term) {
            beta = beta * scale_w[i] + scale_b[i];
        } else {
            beta = beta * scale_w[i];
        }
        for (int ni = 0; ni < n; ++ni){
            for (int j=0; j < hw; j++) {
                weights_p[ni * chw + i * hw + j] *= alpha;
            }
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update fp32 deconv weights with batchnorm.
 */
template<typename T>
void WeightsFusion<float, T>::update_deconv_weights_without_scale(
                    PBlock<T> weights, PBlock<T> bias,
                    int n, int c, int h, int w, bool conv_bias_term,
                    float batchnorm_scale, float batchnorm_eps,
                    std::vector<float> batchnorm_mean,
                    std::vector<float> batchnorm_variance) {
    float* weights_p = (float*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();

    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    //swap n and c
    int tn = c;
    c = n;
    n = tn;

    int chw = c * h * w;
    int hw = h * w;
    for (int i = 0; i < c; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;
        for (int ni = 0; ni < n; ++ni){
            for (int j=0; j < hw; j++){
                weights_p[ni * chw + i * hw + j] *= alpha;
            }
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update int8 conv weights with batchnorm and scale parameters.
 */
template<typename T>
void WeightsFusion<char, T>::update_weights(
        PBlock<T> weights, PBlock<T> bias,
        int n, int c, int h, int w, bool conv_bias_term,
        float batchnorm_scale, float batchnorm_eps,
        std::vector<float> batchnorm_mean,
        std::vector<float> batchnorm_variance,
        std::vector<float> scale_w,
        std::vector<float> scale_b,
        bool scale_bias_term) {
    char* weights_p = (char*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;

        // insert scale parameters
        alpha = scale_w[i] * alpha;
        if (scale_bias_term) {
            beta = beta * scale_w[i] + scale_b[i];
        } else {
            beta = beta * scale_w[i];
        }
        // change weights scale
        w_scale[i] *= alpha;
        if (w_scale[i] < 0){
            w_scale[i] = fabs(w_scale[i]);
            for (int j = 0; j < chw; ++j){
                weights_p[i * chw + j] *= -1;
            }
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.h_tensor().set_scale(w_scale);
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

template<typename T>
void WeightsFusion<char, T>::update_weights_conv_scale(PBlock<T> weights, PBlock<T> bias,
                               int n, int c, int h, int w, bool conv_bias_term,
                               std::vector<float> scale_w,
                               std::vector<float> scale_b,
                               bool scale_bias_term){
    char* weights_p = (char*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, scale_w.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();

    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        float alpha = scale_w[i];
        float beta = 0.f;
        // insert scale parameters
        if (scale_bias_term) {
            beta = scale_b[i];
        }
        int start_index = i * chw;
        for (int j = 0; j < chw; j++) {
            weights_p[start_index + j] *= alpha;
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.h_tensor().set_scale(w_scale);
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update int8 conv weights with affine channel parameters.
 */
template<typename T>
void WeightsFusion<char, T>::update_conv_affine_channel_weights(
        PBlock<T> weights, PBlock<T> bias,
        int n, int c, int h, int w,
        std::vector<float> affine_channel_w,
        std::vector<float> affine_channel_b) {
    char* weights_p = (char*)(weights.h_tensor().mutable_data());
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        // change weights scale
        w_scale[i] *= affine_channel_w[i];
        if (w_scale[i] < 0){
            w_scale[i] = fabs(w_scale[i]);
            for (int j = 0; j < chw; ++j){
                weights_p[i * chw + j] *= -1;
            }
        }
        bias_p[i] = bias_p[i] * affine_channel_w[i] + affine_channel_b[i];
    }
    weights.h_tensor().set_scale(w_scale);
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
 * \brief  update int8 conv weights with batchnorm.
 */
template<typename T>
void WeightsFusion<char, T>::update_weights_without_scale(
        PBlock<T> weights, PBlock<T> bias,
        int n, int c, int h, int w, bool conv_bias_term,
        float batchnorm_scale, float batchnorm_eps,
        std::vector<float> batchnorm_mean,
        std::vector<float> batchnorm_variance) {
    char* weights_p = (char*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    int chw = c * h * w;
    for (int i = 0; i < n; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;

        // change weights scale
        w_scale[i] *= alpha;
        if (w_scale[i] < 0){
            w_scale[i] = fabs(w_scale[i]);
            for (int j = 0; j < chw; ++j){
                int start_index = i * chw;
                weights_p[start_index + j] *= -1;
            }
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.h_tensor().set_scale(w_scale);
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}
/**
 * \brief  update int8 deconv weights with batchnorm and scale parameters.
 */
template<typename T>
void WeightsFusion<char, T>::update_deconv_weights(
        PBlock<T> weights, PBlock<T> bias,
        int n, int c, int h, int w, bool conv_bias_term,
        float batchnorm_scale, float batchnorm_eps,
        std::vector<float> batchnorm_mean,
        std::vector<float> batchnorm_variance,
        std::vector<float> scale_w,
        std::vector<float> scale_b,
        bool scale_bias_term) {
    char* weights_p = (char*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());

    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    //swap n and c
    int tn = c;
    c = n;
    n = tn;

    int chw = c * h * w;
    int hw = h * w;
    for (int i = 0; i < c; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;

        // insert scale parameters
        alpha = scale_w[i] * alpha;
        if (scale_bias_term) {
            beta = beta * scale_w[i] + scale_b[i];
        } else {
            beta = beta * scale_w[i];
        }
        // change weights scale
        w_scale[i] *= alpha;
        if (w_scale[i] < 0){
            w_scale[i] = fabs(w_scale[i]);
            for (int ni = 0; ni < n; ++ni){
                for (int j = 0; j < hw; j++) {
                    weights_p[ni * chw + i * hw + j] *= -1;
                }
            }
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.h_tensor().set_scale(w_scale);
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}

/**
* \brief  update int8 deconv weights with batchnorm.
*/
template<typename T>
void WeightsFusion<char, T>::update_deconv_weights_without_scale(
        PBlock<T> weights, PBlock<T> bias,
        int n, int c, int h, int w, bool conv_bias_term,
        float batchnorm_scale, float batchnorm_eps,
        std::vector<float> batchnorm_mean,
        std::vector<float> batchnorm_variance) {
    char* weights_p = (char*)(weights.h_tensor().mutable_data());
    if (!conv_bias_term) {
        bias.re_alloc(Shape4d({1, batchnorm_mean.size(), 1, 1}));
        void* new_bias_data = bias.h_tensor().mutable_data();
        memset(new_bias_data, 0, sizeof(float) * bias.h_tensor().size());
    }
    float* bias_p = (float*)(bias.h_tensor().mutable_data());

    batchnorm_scale = (batchnorm_scale == 0) ? 1.f : 1.f / batchnorm_scale;
    std::vector<float> w_scale = weights.h_tensor().get_scale();
    //swap n and c
    int tn = c;
    c = n;
    n = tn;

    int chw = c * h * w;
    int hw = h * w;
    for (int i = 0; i < c; i++) {
        float alpha = 1.f;
        float beta = 0.f;
        // insert batchnorm parameters
        alpha = batchnorm_variance[i] * batchnorm_scale + batchnorm_eps;
        alpha = 1.f / sqrtf(alpha);
        beta = -1.f * (batchnorm_mean[i] * batchnorm_scale);
        beta = beta * alpha;
        w_scale[i] *= alpha;
        if (w_scale[i] < 0){
            w_scale[i] = fabs(w_scale[i]);
            for (int ni = 0; ni < n; ++ni){
                for (int j = 0; j < hw; j++) {
                    weights_p[ni * chw + i * hw + j] *= -1;
                }
            }
        }
        bias_p[i] *= alpha;
        bias_p[i] += beta;
    }
    weights.h_tensor().set_scale(w_scale);
    weights.d_tensor().copy_from(weights.h_tensor());
    weights.d_tensor().set_scale(w_scale);
    bias.d_tensor().copy_from(bias.h_tensor());
}
#if defined USE_CUDA
template class WeightsFusion<float, NV>;
template class WeightsFusion<char, NV>;
#endif

#if defined USE_X86_PLACE || defined BUILD_LITE
template class WeightsFusion<float, X86>;
template class WeightsFusion<char, X86>;
#endif
#if defined USE_ARM_PLACE
template class WeightsFusion<float, ARM>;
template class WeightsFusion<char, ARM>;
#endif

}
