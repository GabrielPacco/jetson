#ifndef ENVIRONMENT_LEGO_ROBOT_ENV_H
#define ENVIRONMENT_LEGO_ROBOT_ENV_H

#include "environment/environment_interface.h"
#include "communication/bluetooth_manager.h"
#include <memory>
#include <chrono>

namespace environment {

/**
 * @brief Reward function parameters
 */
struct RewardParams {
    float forward_success = 1.0f;       // Reward for successful forward movement
    float collision_penalty = -1.0f;    // Penalty for collision
    float backward_penalty = -0.1f;     // Penalty for moving backward
    float turn_reward = 0.0f;           // Reward for turning
    float orientation_bonus = 0.5f;     // Bonus for stable orientation
};

/**
 * @brief Lego Robot Environment for DQN Training
 *
 * State space (4D):
 *   - orientation_x: normalized gyroscope X reading (-1 to 1)
 *   - orientation_y: normalized gyroscope Y reading (-1 to 1)
 *   - contact_front: binary contact sensor (0 or 1)
 *   - contact_side: binary contact sensor (0 or 1)
 *
 * Action space (4 discrete):
 *   - 0: Move forward
 *   - 1: Move backward
 *   - 2: Turn left
 *   - 3: Turn right
 */
class LegoRobotEnv : public EnvironmentInterface {
public:
    /**
     * @brief Construct a new Lego Robot Env object
     *
     * @param robot_address Bluetooth MAC address of robot
     * @param max_steps_per_episode Maximum steps before episode ends (default: 200)
     * @param episode_timeout_sec Maximum time per episode in seconds (default: 60)
     * @param reward_params Custom reward parameters (optional)
     */
    LegoRobotEnv(const std::string& robot_address,
                 int max_steps_per_episode = 200,
                 int episode_timeout_sec = 60,
                 const RewardParams& reward_params = RewardParams());

    ~LegoRobotEnv() override;

    // EnvironmentInterface implementation
    torch::Tensor reset() override;
    StepResult step(int64_t action) override;
    int64_t state_dim() const override { return 4; }
    int64_t action_dim() const override { return 4; }
    void close() override;

private:
    /**
     * @brief Get current state from robot sensors
     *
     * @return torch::Tensor State tensor [orientation_x, orientation_y, contact_front, contact_side]
     */
    torch::Tensor get_current_state();

    /**
     * @brief Compute reward based on state and action
     *
     * @param state Current state
     * @param action Action taken
     * @param collision Whether collision occurred
     * @return float Reward value
     */
    float compute_reward(const torch::Tensor& state, int64_t action, bool collision);

    /**
     * @brief Check if episode should end
     *
     * @param state Current state
     * @param step_count Current step count
     * @return true if episode done
     */
    bool is_episode_done(const torch::Tensor& state, int step_count);

    // Bluetooth communication
    std::unique_ptr<communication::BluetoothManager> bt_manager_;

    // Environment parameters
    int max_steps_per_episode_;
    int episode_timeout_sec_;
    RewardParams reward_params_;

    // Episode tracking
    int current_step_;
    std::chrono::steady_clock::time_point episode_start_time_;

    // Previous state for orientation stability tracking
    torch::Tensor previous_state_;
};

} // namespace environment

#endif // ENVIRONMENT_LEGO_ROBOT_ENV_H
