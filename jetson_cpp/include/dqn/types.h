#ifndef DQN_TYPES_H
#define DQN_TYPES_H

#include <torch/torch.h>
#include <vector>
#include <string>

namespace dqn {

/**
 * @brief Hyperparameters for DQN algorithm
 *
 * Contains all configurable parameters for training the DQN agent.
 */
struct Hyperparameters {
    // Learning parameters
    float learning_rate = 0.001f;           // Adam optimizer learning rate
    float gamma = 0.99f;                    // Discount factor for future rewards

    // Exploration parameters
    float epsilon_start = 1.0f;             // Initial exploration probability
    float epsilon_end = 0.05f;              // Minimum exploration probability
    float epsilon_decay = 0.995f;           // Exponential decay factor for epsilon

    // Network update parameters
    int64_t target_update_freq = 10;        // Update target network every N episodes

    // Replay buffer parameters
    size_t batch_size = 64;                 // Minibatch size for training
    size_t buffer_capacity = 10000;         // Maximum replay buffer capacity

    // Network architecture
    int64_t hidden_dim1 = 128;              // First hidden layer dimension
    int64_t hidden_dim2 = 128;              // Second hidden layer dimension
};

/**
 * @brief Single transition in the environment
 *
 * Represents one step of interaction: (state, action, reward, next_state, done)
 */
struct Transition {
    torch::Tensor state;         // Current state
    int64_t action;              // Action taken
    float reward;                // Reward received
    torch::Tensor next_state;    // Resulting next state
    bool done;                   // Whether episode ended
};

/**
 * @brief Batch of transitions for training
 *
 * Contains batched tensors for efficient GPU processing
 */
struct TransitionBatch {
    torch::Tensor states;        // [batch_size, state_dim]
    torch::Tensor actions;       // [batch_size, 1]
    torch::Tensor rewards;       // [batch_size, 1]
    torch::Tensor next_states;   // [batch_size, state_dim]
    torch::Tensor dones;         // [batch_size, 1]
};

} // namespace dqn

#endif // DQN_TYPES_H
