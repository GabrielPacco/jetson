#ifndef ENVIRONMENT_INTERFACE_H
#define ENVIRONMENT_INTERFACE_H

#include <torch/torch.h>
#include <string>

namespace environment {

/**
 * @brief Result of an environment step
 */
struct StepResult {
    torch::Tensor next_state;    // Resulting state after action
    float reward;                // Reward received
    bool done;                   // Whether episode ended
    std::string info;            // Additional information (for debugging)
};

/**
 * @brief Abstract interface for reinforcement learning environments
 *
 * Follows the OpenAI Gym interface pattern for consistency.
 */
class EnvironmentInterface {
public:
    virtual ~EnvironmentInterface() = default;

    /**
     * @brief Reset the environment to initial state
     *
     * @return torch::Tensor Initial state
     */
    virtual torch::Tensor reset() = 0;

    /**
     * @brief Execute an action in the environment
     *
     * @param action Action to execute
     * @return StepResult Result of the step
     */
    virtual StepResult step(int64_t action) = 0;

    /**
     * @brief Get dimension of state space
     *
     * @return int64_t State dimension
     */
    virtual int64_t state_dim() const = 0;

    /**
     * @brief Get number of discrete actions
     *
     * @return int64_t Action count
     */
    virtual int64_t action_dim() const = 0;

    /**
     * @brief Close the environment and clean up resources
     */
    virtual void close() = 0;
};

} // namespace environment

#endif // ENVIRONMENT_INTERFACE_H
