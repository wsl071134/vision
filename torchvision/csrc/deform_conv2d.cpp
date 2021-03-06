#include "deform_conv2d.h"
#include <torch/extension.h>

#if defined(WITH_CUDA) || defined(WITH_HIP)
#include <ATen/autocast_mode.h>
#endif

namespace vision {
namespace ops {

at::Tensor deform_conv2d(
    const at::Tensor& input,
    const at::Tensor& weight,
    const at::Tensor& offset,
    const at::Tensor& mask,
    const at::Tensor& bias,
    int64_t stride_h,
    int64_t stride_w,
    int64_t pad_h,
    int64_t pad_w,
    int64_t dilation_h,
    int64_t dilation_w,
    int64_t groups,
    int64_t offset_groups,
    bool use_mask) {
  static auto op = c10::Dispatcher::singleton()
                       .findSchemaOrThrow("torchvision::deform_conv2d", "")
                       .typed<decltype(deform_conv2d)>();
  return op.call(
      input,
      weight,
      offset,
      mask,
      bias,
      stride_h,
      stride_w,
      pad_h,
      pad_w,
      dilation_h,
      dilation_w,
      groups,
      offset_groups,
      use_mask);
}

#if defined(WITH_CUDA) || defined(WITH_HIP)
at::Tensor deform_conv2d_autocast(
    const at::Tensor& input,
    const at::Tensor& weight,
    const at::Tensor& offset,
    const at::Tensor& mask,
    const at::Tensor& bias,
    int64_t stride_h,
    int64_t stride_w,
    int64_t pad_h,
    int64_t pad_w,
    int64_t dilation_h,
    int64_t dilation_w,
    int64_t groups,
    int64_t offset_groups,
    bool use_mask) {
  c10::impl::ExcludeDispatchKeyGuard no_autocast(c10::DispatchKey::Autocast);
  return deform_conv2d(
             at::autocast::cached_cast(at::kFloat, input),
             at::autocast::cached_cast(at::kFloat, weight),
             at::autocast::cached_cast(at::kFloat, offset),
             at::autocast::cached_cast(at::kFloat, mask),
             at::autocast::cached_cast(at::kFloat, bias),
             stride_h,
             stride_w,
             pad_h,
             pad_w,
             dilation_h,
             dilation_w,
             groups,
             offset_groups,
             use_mask)
      .to(input.scalar_type());
}
#endif

std::tuple<at::Tensor, at::Tensor, at::Tensor, at::Tensor, at::Tensor>
_deform_conv2d_backward(
    const at::Tensor& grad,
    const at::Tensor& input,
    const at::Tensor& weight,
    const at::Tensor& offset,
    const at::Tensor& mask,
    const at::Tensor& bias,
    int64_t stride_h,
    int64_t stride_w,
    int64_t pad_h,
    int64_t pad_w,
    int64_t dilation_h,
    int64_t dilation_w,
    int64_t groups,
    int64_t offset_groups,
    bool use_mask) {
  static auto op =
      c10::Dispatcher::singleton()
          .findSchemaOrThrow("torchvision::_deform_conv2d_backward", "")
          .typed<decltype(_deform_conv2d_backward)>();
  return op.call(
      grad,
      input,
      weight,
      offset,
      mask,
      bias,
      stride_h,
      stride_w,
      pad_h,
      pad_w,
      dilation_h,
      dilation_w,
      groups,
      offset_groups,
      use_mask);
}

namespace {

class DeformConv2dFunction
    : public torch::autograd::Function<DeformConv2dFunction> {
 public:
  static torch::autograd::variable_list forward(
      torch::autograd::AutogradContext* ctx,
      const torch::autograd::Variable& input,
      const torch::autograd::Variable& weight,
      const torch::autograd::Variable& offset,
      const torch::autograd::Variable& mask,
      const torch::autograd::Variable& bias,
      int64_t stride_h,
      int64_t stride_w,
      int64_t pad_h,
      int64_t pad_w,
      int64_t dilation_h,
      int64_t dilation_w,
      int64_t groups,
      int64_t offset_groups,
      bool use_mask) {
    at::AutoNonVariableTypeMode g;
    auto output = deform_conv2d(
        input,
        weight,
        offset,
        mask,
        bias,
        stride_h,
        stride_w,
        pad_h,
        pad_w,
        dilation_h,
        dilation_w,
        groups,
        offset_groups,
        use_mask);

    ctx->save_for_backward({input, weight, offset, mask, bias});
    ctx->saved_data["stride_h"] = stride_h;
    ctx->saved_data["stride_w"] = stride_w;
    ctx->saved_data["pad_h"] = pad_h;
    ctx->saved_data["pad_w"] = pad_w;
    ctx->saved_data["dilation_h"] = dilation_h;
    ctx->saved_data["dilation_w"] = dilation_w;
    ctx->saved_data["groups"] = groups;
    ctx->saved_data["offset_groups"] = offset_groups;
    ctx->saved_data["use_mask"] = use_mask;

    return {
        output,
    };
  }

  static torch::autograd::variable_list backward(
      torch::autograd::AutogradContext* ctx,
      const torch::autograd::variable_list& grad_output) {
    auto saved = ctx->get_saved_variables();
    auto input = saved[0];
    auto weight = saved[1];
    auto offset = saved[2];
    auto mask = saved[3];
    auto bias = saved[4];

    auto stride_h = ctx->saved_data["stride_h"].toInt();
    auto stride_w = ctx->saved_data["stride_w"].toInt();
    auto pad_h = ctx->saved_data["pad_h"].toInt();
    auto pad_w = ctx->saved_data["pad_w"].toInt();
    auto dilation_h = ctx->saved_data["dilation_h"].toInt();
    auto dilation_w = ctx->saved_data["dilation_w"].toInt();
    auto groups = ctx->saved_data["groups"].toInt();
    auto offset_groups = ctx->saved_data["offset_groups"].toInt();
    auto use_mask = ctx->saved_data["use_mask"].toBool();

    auto grads = _deform_conv2d_backward(
        grad_output[0],
        input,
        weight,
        offset,
        mask,
        bias,
        stride_h,
        stride_w,
        pad_h,
        pad_w,
        dilation_h,
        dilation_w,
        groups,
        offset_groups,
        use_mask);
    auto grad_input = std::get<0>(grads);
    auto grad_weight = std::get<1>(grads);
    auto grad_offset = std::get<2>(grads);
    auto grad_mask = std::get<3>(grads);
    auto grad_bias = std::get<4>(grads);

    return {
        grad_input,
        grad_weight,
        grad_offset,
        grad_mask,
        grad_bias,
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
        torch::autograd::Variable(),
    };
  }
};

// TODO: There should be an easier way to do this
class DeformConv2dBackwardFunction
    : public torch::autograd::Function<DeformConv2dBackwardFunction> {
 public:
  static torch::autograd::variable_list forward(
      torch::autograd::AutogradContext* ctx,
      const torch::autograd::Variable& grad,
      const torch::autograd::Variable& input,
      const torch::autograd::Variable& weight,
      const torch::autograd::Variable& offset,
      const torch::autograd::Variable& mask,
      const torch::autograd::Variable& bias,
      int64_t stride_h,
      int64_t stride_w,
      int64_t pad_h,
      int64_t pad_w,
      int64_t dilation_h,
      int64_t dilation_w,
      int64_t groups,
      int64_t offset_groups,
      bool use_mask) {
    at::AutoNonVariableTypeMode g;
    auto result = _deform_conv2d_backward(
        grad,
        input,
        weight,
        offset,
        mask,
        bias,
        stride_h,
        stride_w,
        pad_h,
        pad_w,
        dilation_h,
        dilation_w,
        groups,
        offset_groups,
        use_mask);

    auto grad_input = std::get<0>(result);
    auto grad_weight = std::get<1>(result);
    auto grad_offset = std::get<2>(result);
    auto grad_mask = std::get<3>(result);
    auto grad_bias = std::get<4>(result);

    return {
        grad_input,
        grad_weight,
        grad_offset,
        grad_mask,
        grad_bias,
    };
  }

  static torch::autograd::variable_list backward(
      torch::autograd::AutogradContext* ctx,
      const torch::autograd::variable_list& grad_output) {
    TORCH_CHECK(0, "double backwards on deform_conv2d not supported");
  }
};

} // namespace

at::Tensor deform_conv2d_autograd(
    const at::Tensor& input,
    const at::Tensor& weight,
    const at::Tensor& offset,
    const at::Tensor& mask,
    const at::Tensor& bias,
    int64_t stride_h,
    int64_t stride_w,
    int64_t pad_h,
    int64_t pad_w,
    int64_t dilation_h,
    int64_t dilation_w,
    int64_t groups,
    int64_t offset_groups,
    bool use_mask) {
  return DeformConv2dFunction::apply(
      input,
      weight,
      offset,
      mask,
      bias,
      stride_h,
      stride_w,
      pad_h,
      pad_w,
      dilation_h,
      dilation_w,
      groups,
      offset_groups,
      use_mask)[0];
}

std::tuple<at::Tensor, at::Tensor, at::Tensor, at::Tensor, at::Tensor>
deform_conv2d_backward_autograd(
    const at::Tensor& grad,
    const at::Tensor& input,
    const at::Tensor& weight,
    const at::Tensor& offset,
    const at::Tensor& mask,
    const at::Tensor& bias,
    int64_t stride_h,
    int64_t stride_w,
    int64_t pad_h,
    int64_t pad_w,
    int64_t dilation_h,
    int64_t dilation_w,
    int64_t groups,
    int64_t offset_groups,
    bool use_mask) {
  auto result = DeformConv2dBackwardFunction::apply(
      grad,
      input,
      weight,
      offset,
      mask,
      bias,
      stride_h,
      stride_w,
      pad_h,
      pad_w,
      dilation_h,
      dilation_w,
      groups,
      offset_groups,
      use_mask);

  return std::make_tuple(result[0], result[1], result[2], result[3], result[4]);
}

} // namespace ops
} // namespace vision
