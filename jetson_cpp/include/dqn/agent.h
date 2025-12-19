#ifndef DQN_AGENT_H
#define DQN_AGENT_H

#include <torch/torch.h>
#include <memory>
#include <string>
#include "dqn/network.h"
#include "dqn/replay_buffer.h"
#include "dqn/types.h"

namespace dqn {

/**
 * @brief Deep Q-Network Agent
 *
 * Implements the DQN algorithm with:
 * - Q-network and target network
 * - Epsilon-greedy exploration
 * - Experience replay
 * - Periodic target network updates
 */
class DQNAgent {
public:
    /**
     * @brief Construct a new DQN Agent
     *
     * @param state_dim Dimension of state space
     * @param action_dim Number of discrete actions
     * @param params Hyperparameters for training
     * @param device Device to run on (CPU/CUDA)
     */
    DQNAgent(int64_t state_dim, int64_t action_dim,
             const Hyperparameters& params, torch::Device device);

    /**
     * @brief Select an action using epsilon-greedy policy
     *
     * @param state Current state tensor
     * @param training If true, use epsilon-greedy; if false, use greedy policy
     * @return int64_t Selected action index
     */
    int64_t select_action(const torch::Tensor& state, bool training = true);

    /**
     * @brief Store a transition in the replay buffer
     *
     * @param state Current state
     * @param action Action taken
     * @param reward Reward received
     * @param next_state Next state
     * @param done Whether episode ended
     */
    void store_transition(const torch::Tensor& state, int64_t action, float reward,
                         const torch::Tensor& next_state, bool done);

    /**
     * @brief Perform one training step
     *
     * Samples a batch from replay buffer and performs gradient descent.
     *
     * @return float Loss value (or -1.0 if not enough samples)
     */
    float train_step();

    /**
     * @brief Update target network by copying weights from Q-network
     */
    void update_target_network();

    /**
     * @brief Decay epsilon for exploration-exploitation tradeoff
     */
    void decay_epsilon();

    /**
     * @brief Save model to file
     *
     * @param filepath Path to save file (.pt extension)
     */
    void save(const std::string& filepath);

    /**
     * @brief Load model from file
     *
     * @param filepath Path to model file
     */
    void load(const std::string& filepath);

    /**
     * @brief Get current epsilon value
     *
     * @return float Current epsilon
     */
    float get_epsilon() const { return epsilon_; }

    /**
     * @brief Get number of training steps performed
     *
     * @return int64_t Training step count
     */
    int64_t get_training_steps() const { return training_steps_; }

    /**
     * @brief Set evaluation mode (disable epsilon-greedy)
     */
    void eval();

    /**
     * @brief Set training mode (enable epsilon-greedy)
     */
    void train();

private:
    // Neural networks
    QNetwork q_network_{nullptr};                 // Policy network
    QNetwork target_network_{nullptr};            // Target network

    // Optimizer
    std::unique_ptr<torch::optim::Adam> optimizer_;

    // Replay buffer
    std::unique_ptr<ReplayBuffer> replay_buffer_;

    // Hyperparameters
    Hyperparameters params_;

    // Device (CPU/CUDA)
    torch::Device device_;

    // State dimensions
    int64_t state_dim_;
    int64_t action_dim_;

    // Exploration
    float epsilon_;

    // Training progress
    int64_t training_steps_;

    // Random number generator for epsilon-greedy
    std::mt19937 rng_;
    std::uniform_real_distribution<float> uniform_dist_;
};

} // namespace dqn

#endif // DQN_AGENT_H
