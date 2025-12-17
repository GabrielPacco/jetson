#include "dqn/replay_buffer.h"
#include <stdexcept>
#include <algorithm>

namespace dqn {

ReplayBuffer::ReplayBuffer(size_t capacity)
    : capacity_(capacity), rng_(std::random_device{}()) {
    // Note: std::deque doesn't have reserve(), but handles memory efficiently
}

void ReplayBuffer::push(const torch::Tensor& state, int64_t action, float reward,
                       const torch::Tensor& next_state, bool done) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Create transition
    Transition transition;
    transition.state = state.clone().detach();           // Clone to avoid aliasing
    transition.action = action;
    transition.reward = reward;
    transition.next_state = next_state.clone().detach(); // Clone to avoid aliasing
    transition.done = done;

    // Add to buffer
    buffer_.push_back(std::move(transition));

    // Remove oldest if over capacity (FIFO)
    if (buffer_.size() > capacity_) {
        buffer_.pop_front();
    }
}

TransitionBatch ReplayBuffer::sample(size_t batch_size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (buffer_.size() < batch_size) {
        throw std::runtime_error("ReplayBuffer: Not enough transitions to sample. "
                                "Buffer size: " + std::to_string(buffer_.size()) +
                                ", requested: " + std::to_string(batch_size));
    }

    // Generate random indices for sampling
    std::vector<size_t> indices(buffer_.size());
    for (size_t i = 0; i < buffer_.size(); ++i) {
        indices[i] = i;
    }

    // Shuffle and take first batch_size indices
    std::shuffle(indices.begin(), indices.end(), rng_);
    indices.resize(batch_size);

    // Collect sampled transitions
    std::vector<torch::Tensor> states;
    std::vector<int64_t> actions;
    std::vector<float> rewards;
    std::vector<torch::Tensor> next_states;
    std::vector<float> dones;

    states.reserve(batch_size);
    actions.reserve(batch_size);
    rewards.reserve(batch_size);
    next_states.reserve(batch_size);
    dones.reserve(batch_size);

    for (size_t idx : indices) {
        const Transition& t = buffer_[idx];
        states.push_back(t.state);
        actions.push_back(t.action);
        rewards.push_back(t.reward);
        next_states.push_back(t.next_state);
        dones.push_back(t.done ? 1.0f : 0.0f);
    }

    // Convert to batched tensors
    TransitionBatch batch;

    // Stack states: [batch_size, state_dim]
    batch.states = torch::stack(states);

    // Actions: [batch_size, 1]
    batch.actions = torch::tensor(actions, torch::kLong).unsqueeze(1);

    // Rewards: [batch_size, 1]
    batch.rewards = torch::tensor(rewards, torch::kFloat32).unsqueeze(1);

    // Stack next_states: [batch_size, state_dim]
    batch.next_states = torch::stack(next_states);

    // Dones: [batch_size, 1]
    batch.dones = torch::tensor(dones, torch::kFloat32).unsqueeze(1);

    return batch;
}

size_t ReplayBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size();
}

bool ReplayBuffer::can_sample(size_t batch_size) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size() >= batch_size;
}

void ReplayBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
}

} // namespace dqn
