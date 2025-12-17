#include "dqn/agent.h"
#include <iostream>
#include <random>

namespace dqn {

DQNAgent::DQNAgent(int64_t state_dim, int64_t action_dim,
                   const Hyperparameters& params, torch::Device device)
    : params_(params),
      device_(device),
      state_dim_(state_dim),
      action_dim_(action_dim),
      epsilon_(params.epsilon_start),
      training_steps_(0),
      rng_(std::random_device{}()),
      uniform_dist_(0.0f, 1.0f),
      q_network_(nullptr),
      target_network_(nullptr) {

    // Create Q-network and target network
    q_network_ = QNetwork(state_dim, action_dim, params.hidden_dim1, params.hidden_dim2);
    target_network_ = QNetwork(state_dim, action_dim, params.hidden_dim1, params.hidden_dim2);

    // Move networks to device
    q_network_->to(device);
    target_network_->to(device);

    // Initialize target network with same weights as Q-network
    update_target_network();

    // Set target network to eval mode (no dropout, batchnorm in eval mode)
    target_network_->eval();

    // Create optimizer for Q-network
    optimizer_ = std::make_unique<torch::optim::Adam>(
        q_network_->parameters(),
        torch::optim::AdamOptions(params.learning_rate)
    );

    // Create replay buffer
    replay_buffer_ = std::make_unique<ReplayBuffer>(params.buffer_capacity);

    std::cout << "[DQNAgent] Initialized with:" << std::endl;
    std::cout << "  State dim: " << state_dim << std::endl;
    std::cout << "  Action dim: " << action_dim << std::endl;
    std::cout << "  Hidden dims: [" << params.hidden_dim1 << ", " << params.hidden_dim2 << "]" << std::endl;
    std::cout << "  Device: " << device << std::endl;
    std::cout << "  Learning rate: " << params.learning_rate << std::endl;
    std::cout << "  Gamma: " << params.gamma << std::endl;
    std::cout << "  Epsilon: " << epsilon_ << " -> " << params.epsilon_end << std::endl;
}

int64_t DQNAgent::select_action(const torch::Tensor& state, bool training) {
    // Epsilon-greedy exploration during training
    if (training && uniform_dist_(rng_) < epsilon_) {
        // Random action
        std::uniform_int_distribution<int64_t> action_dist(0, action_dim_ - 1);
        return action_dist(rng_);
    }

    // Greedy action: select action with highest Q-value
    torch::NoGradGuard no_grad;  // Disable gradient computation for inference

    // Move state to device and ensure correct shape
    torch::Tensor state_tensor = state.to(device_);
    if (state_tensor.dim() == 1) {
        state_tensor = state_tensor.unsqueeze(0);  // Add batch dimension
    }

    // Forward pass through Q-network
    torch::Tensor q_values = q_network_->forward(state_tensor);

    // Select action with maximum Q-value
    int64_t action = q_values.argmax(1).item<int64_t>();

    return action;
}

void DQNAgent::store_transition(const torch::Tensor& state, int64_t action, float reward,
                                const torch::Tensor& next_state, bool done) {
    replay_buffer_->push(state, action, reward, next_state, done);
}

float DQNAgent::train_step() {
    // Check if we have enough samples in the buffer
    if (!replay_buffer_->can_sample(params_.batch_size)) {
        return -1.0f;  // Not enough samples yet
    }

    // Sample a batch from replay buffer
    TransitionBatch batch = replay_buffer_->sample(params_.batch_size);

    // Move batch tensors to device
    batch.states = batch.states.to(device_);
    batch.actions = batch.actions.to(device_);
    batch.rewards = batch.rewards.to(device_);
    batch.next_states = batch.next_states.to(device_);
    batch.dones = batch.dones.to(device_);

    // ========== Compute Current Q-values ==========
    // Q(s, a) for the actions that were taken
    torch::Tensor q_values = q_network_->forward(batch.states);  // [batch_size, action_dim]
    torch::Tensor current_q_values = q_values.gather(1, batch.actions);  // [batch_size, 1]

    // ========== Compute Target Q-values ==========
    torch::NoGradGuard no_grad;  // No gradients for target network

    // Target: r + gamma * max_a' Q_target(s', a') * (1 - done)
    torch::Tensor next_q_values = target_network_->forward(batch.next_states);  // [batch_size, action_dim]
    torch::Tensor max_next_q_values = std::get<0>(next_q_values.max(1, true));  // [batch_size, 1]

    // Apply Bellman equation
    torch::Tensor target_q_values = batch.rewards +
                                    params_.gamma * max_next_q_values * (1.0f - batch.dones);

    no_grad.~NoGradGuard();  // Re-enable gradients

    // ========== Compute Loss ==========
    // Mean Squared Error between current Q-values and target Q-values
    torch::Tensor loss = torch::mse_loss(current_q_values, target_q_values);

    // ========== Backpropagation ==========
    optimizer_->zero_grad();  // Reset gradients
    loss.backward();          // Compute gradients
    optimizer_->step();       // Update weights

    training_steps_++;

    return loss.item<float>();
}

void DQNAgent::update_target_network() {
    // Copy weights from Q-network to target network
    torch::NoGradGuard no_grad;

    auto q_params = q_network_->named_parameters();
    auto target_params = target_network_->named_parameters();

    for (auto& q_pair : q_params) {
        auto& name = q_pair.key();
        auto& q_param = q_pair.value();

        if (target_params.contains(name)) {
            target_params[name].copy_(q_param);
        }
    }

    std::cout << "[DQNAgent] Target network updated at step " << training_steps_ << std::endl;
}

void DQNAgent::decay_epsilon() {
    epsilon_ = std::max(params_.epsilon_end, epsilon_ * params_.epsilon_decay);
}

void DQNAgent::save(const std::string& filepath) {
    try {
        torch::serialize::OutputArchive archive;

        // Save Q-network parameters
        q_network_->save(archive);

        // Save optimizer state
        torch::serialize::OutputArchive optimizer_archive;
        optimizer_->save(optimizer_archive);

        // Create a dictionary to save everything
        torch::Dict<std::string, torch::Tensor> state_dict;

        // Save epsilon and training steps as tensors
        state_dict.insert("epsilon", torch::tensor(epsilon_));
        state_dict.insert("training_steps", torch::tensor(static_cast<int64_t>(training_steps_)));

        // Save to file
        torch::save(q_network_, filepath);

        std::cout << "[DQNAgent] Model saved to: " << filepath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DQNAgent] Error saving model: " << e.what() << std::endl;
    }
}

void DQNAgent::load(const std::string& filepath) {
    try {
        // Load model
        torch::load(q_network_, filepath);

        // Move to device
        q_network_->to(device_);

        // Update target network
        update_target_network();

        std::cout << "[DQNAgent] Model loaded from: " << filepath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DQNAgent] Error loading model: " << e.what() << std::endl;
    }
}

void DQNAgent::eval() {
    q_network_->eval();
    target_network_->eval();
}

void DQNAgent::train() {
    q_network_->train();
    target_network_->eval();  // Target network always in eval mode
}

} // namespace dqn
