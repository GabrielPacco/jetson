#ifndef DQN_NETWORK_H
#define DQN_NETWORK_H

#include <torch/torch.h>

namespace dqn {

/**
 * @brief Deep Q-Network implementation using LibTorch
 *
 * Neural network that approximates the Q-function Q(s, a).
 * Architecture: state_dim -> hidden_dim1 -> hidden_dim2 -> action_dim
 * Uses ReLU activations between layers.
 */
class QNetworkImpl : public torch::nn::Module {
public:
    /**
     * @brief Construct a new QNetwork object
     *
     * @param state_dim Dimension of the state space
     * @param action_dim Number of discrete actions
     * @param hidden_dim1 Size of first hidden layer (default: 128)
     * @param hidden_dim2 Size of second hidden layer (default: 128)
     */
    QNetworkImpl(int64_t state_dim, int64_t action_dim,
                 int64_t hidden_dim1 = 128, int64_t hidden_dim2 = 128);

    /**
     * @brief Forward pass through the network
     *
     * @param x Input state tensor [batch_size, state_dim]
     * @return Q-values for each action [batch_size, action_dim]
     */
    torch::Tensor forward(torch::Tensor x);

    /**
     * @brief Move network to specified device (CPU/CUDA)
     *
     * @param device Target device (torch::kCPU or torch::kCUDA)
     */
    void to_device(torch::Device device);

private:
    // Network layers
    torch::nn::Linear fc1_{nullptr};    // First fully connected layer
    torch::nn::Linear fc2_{nullptr};    // Second fully connected layer
    torch::nn::Linear fc3_{nullptr};    // Output layer

    // Store dimensions for reference
    int64_t state_dim_;
    int64_t action_dim_;
    int64_t hidden_dim1_;
    int64_t hidden_dim2_;
};

// Macro to create a shared pointer wrapper for the network
// This allows using QNetwork as a module handle
TORCH_MODULE(QNetwork);

} // namespace dqn

#endif // DQN_NETWORK_H
