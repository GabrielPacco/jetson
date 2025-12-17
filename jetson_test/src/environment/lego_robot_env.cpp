#include "environment/lego_robot_env.h"
#include <iostream>
#include <cmath>
#include <stdexcept>

namespace environment {

LegoRobotEnv::LegoRobotEnv(const std::string& robot_address,
                           int max_steps_per_episode,
                           int episode_timeout_sec,
                           const RewardParams& reward_params)
    : max_steps_per_episode_(max_steps_per_episode),
      episode_timeout_sec_(episode_timeout_sec),
      reward_params_(reward_params),
      current_step_(0) {

    std::cout << "[LegoRobotEnv] Initializing environment..." << std::endl;
    std::cout << "  Robot address: " << robot_address << std::endl;
    std::cout << "  Max steps per episode: " << max_steps_per_episode_ << std::endl;
    std::cout << "  Episode timeout: " << episode_timeout_sec_ << " seconds" << std::endl;

    // Create Bluetooth manager
    bt_manager_ = std::make_unique<communication::BluetoothManager>(robot_address);

    // Connect to robot
    if (!bt_manager_->connect()) {
        throw std::runtime_error("Failed to connect to robot");
    }

    std::cout << "[LegoRobotEnv] Environment initialized successfully" << std::endl;
}

LegoRobotEnv::~LegoRobotEnv() {
    close();
}

torch::Tensor LegoRobotEnv::reset() {
    std::cout << "[LegoRobotEnv] Resetting environment (Episode start)" << std::endl;

    // Reset episode tracking
    current_step_ = 0;
    episode_start_time_ = std::chrono::steady_clock::now();

    // Stop robot (send backward command briefly to ensure it's stopped)
    bt_manager_->send_command(communication::ACTION_BACKWARD, 10);

    // Wait a moment for robot to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Get initial state
    torch::Tensor initial_state = get_current_state();
    previous_state_ = initial_state.clone();

    std::cout << "[LegoRobotEnv] Initial state: " << initial_state << std::endl;

    return initial_state;
}

StepResult LegoRobotEnv::step(int64_t action) {
    current_step_++;

    // Send action command to robot
    uint8_t action_code = static_cast<uint8_t>(action);
    bool command_sent = bt_manager_->send_command(action_code, communication::DEFAULT_ACTION_DURATION_MS);

    if (!command_sent) {
        throw std::runtime_error("Failed to send command to robot");
    }

    // Wait for action to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(communication::DEFAULT_ACTION_DURATION_MS + 50));

    // Read new state from sensors
    torch::Tensor next_state;
    try {
        next_state = get_current_state();
    } catch (const std::exception& e) {
        std::cerr << "[LegoRobotEnv] Error reading sensors: " << e.what() << std::endl;
        throw;
    }

    // Check for collision
    bool collision = (next_state[2].item<int>() == 1) || (next_state[3].item<int>() == 1);

    // Compute reward
    float reward = compute_reward(next_state, action, collision);

    // Check if episode is done
    bool done = is_episode_done(next_state, current_step_);

    // Create info string
    std::string info = "step=" + std::to_string(current_step_) +
                      ", action=" + communication::action_code_to_name(action_code) +
                      ", collision=" + (collision ? "true" : "false");

    // Update previous state
    previous_state_ = next_state.clone();

    // Create and return result
    StepResult result;
    result.next_state = next_state;
    result.reward = reward;
    result.done = done;
    result.info = info;

    return result;
}

void LegoRobotEnv::close() {
    if (bt_manager_) {
        std::cout << "[LegoRobotEnv] Closing environment" << std::endl;

        // Stop robot before disconnecting
        if (bt_manager_->is_connected()) {
            bt_manager_->send_command(communication::ACTION_BACKWARD, 10);
        }

        bt_manager_->disconnect();
    }
}

torch::Tensor LegoRobotEnv::get_current_state() {
    // Read sensor data from robot
    communication::SensorData sensor_data = bt_manager_->read_sensors();

    // Create state tensor: [orientation_x, orientation_y, contact_front, contact_side]
    torch::Tensor state = torch::zeros({4}, torch::kFloat32);

    // Gyroscope readings (already normalized to -1.0 to 1.0)
    state[0] = sensor_data.gyro_x;
    state[1] = sensor_data.gyro_y;

    // Contact sensors (binary: 0 or 1)
    state[2] = static_cast<float>(sensor_data.contact_front);
    state[3] = static_cast<float>(sensor_data.contact_side);

    return state;
}

float LegoRobotEnv::compute_reward(const torch::Tensor& state, int64_t action, bool collision) {
    float reward = 0.0f;

    // Collision penalty (highest priority)
    if (collision) {
        reward += reward_params_.collision_penalty;
        return reward;  // Return immediately - collision is terminal
    }

    // Action-specific rewards
    switch (action) {
        case communication::ACTION_FORWARD:
            // Reward for successful forward movement (no collision)
            reward += reward_params_.forward_success;
            break;

        case communication::ACTION_BACKWARD:
            // Penalty for backward movement (discourage retreat)
            reward += reward_params_.backward_penalty;
            break;

        case communication::ACTION_LEFT:
        case communication::ACTION_RIGHT:
            // Neutral or small reward for turning
            reward += reward_params_.turn_reward;
            break;
    }

    // Orientation stability bonus
    // Reward stable orientation (low gyroscope values)
    float orientation_magnitude = std::sqrt(
        state[0].item<float>() * state[0].item<float>() +
        state[1].item<float>() * state[1].item<float>()
    );

    if (orientation_magnitude < 0.3f) {
        reward += reward_params_.orientation_bonus;
    }

    return reward;
}

bool LegoRobotEnv::is_episode_done(const torch::Tensor& state, int step_count) {
    // Collision detected
    if (state[2].item<int>() == 1 || state[3].item<int>() == 1) {
        std::cout << "[LegoRobotEnv] Episode ended: Collision detected" << std::endl;
        return true;
    }

    // Maximum steps reached
    if (step_count >= max_steps_per_episode_) {
        std::cout << "[LegoRobotEnv] Episode ended: Max steps reached" << std::endl;
        return true;
    }

    // Timeout
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - episode_start_time_).count();
    if (elapsed >= episode_timeout_sec_) {
        std::cout << "[LegoRobotEnv] Episode ended: Timeout" << std::endl;
        return true;
    }

    return false;
}

} // namespace environment
