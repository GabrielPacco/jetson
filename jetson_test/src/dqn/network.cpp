#include "dqn/network.h"
#include <torch/torch.h>

namespace dqn {

QNetworkImpl::QNetworkImpl(int64_t state_dim, int64_t action_dim,
                           int64_t hidden_dim1, int64_t hidden_dim2)
    : state_dim_(state_dim),
      action_dim_(action_dim),
      hidden_dim1_(hidden_dim1),
      hidden_dim2_(hidden_dim2),
      fc1_(nullptr),
      fc2_(nullptr),
      fc3_(nullptr) {

    // Initialize network layers
    // Layer 1: state_dim -> hidden_dim1
    fc1_ = register_module("fc1", torch::nn::Linear(state_dim, hidden_dim1));

    // Layer 2: hidden_dim1 -> hidden_dim2
    fc2_ = register_module("fc2", torch::nn::Linear(hidden_dim1, hidden_dim2));

    // Output layer: hidden_dim2 -> action_dim
    fc3_ = register_module("fc3", torch::nn::Linear(hidden_dim2, action_dim));

    // Initialize weights using Xavier/Glorot initialization
    // This helps with training stability
    torch::nn::init::xavier_uniform_(fc1_->weight);
    torch::nn::init::constant_(fc1_->bias, 0.0);

    torch::nn::init::xavier_uniform_(fc2_->weight);
    torch::nn::init::constant_(fc2_->bias, 0.0);

    torch::nn::init::xavier_uniform_(fc3_->weight);
    torch::nn::init::constant_(fc3_->bias, 0.0);
}

torch::Tensor QNetworkImpl::forward(torch::Tensor x) {
    // Ensure input has correct shape
    // If input is 1D (single state), add batch dimension
    if (x.dim() == 1) {
        x = x.unsqueeze(0);
    }

    // Layer 1: Linear -> ReLU
    x = torch::relu(fc1_->forward(x));

    // Layer 2: Linear -> ReLU
    x = torch::relu(fc2_->forward(x));

    // Output layer: Linear (no activation)
    // We don't apply softmax or any activation because we want raw Q-values
    x = fc3_->forward(x);

    return x;
}

void QNetworkImpl::to_device(torch::Device device) {
    // Move all parameters and buffers to the specified device
    this->to(device);
}

} // namespace dqn
