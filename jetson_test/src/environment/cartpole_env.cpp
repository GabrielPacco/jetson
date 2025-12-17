#include "environment/cartpole_env.h"
#include <cmath>
#include <iostream>

namespace environment {

CartPoleEnv::CartPoleEnv(int max_steps)
    : max_steps_(max_steps), step_count_(0), rng_(std::random_device{}()) {
    std::cout << "[CartPoleEnv] Entorno de simulación inicializado (sin hardware)" << std::endl;
}

torch::Tensor CartPoleEnv::reset() {
    std::uniform_real_distribution<float> dist(-0.05f, 0.05f);
    x_ = dist(rng_);
    x_dot_ = dist(rng_);
    theta_ = dist(rng_);
    theta_dot_ = dist(rng_);
    step_count_ = 0;
    return get_state();
}

StepResult CartPoleEnv::step(int64_t action) {
    step_count_++;
    float force = (action == 1) ? FORCE_MAG : -FORCE_MAG;
    update_physics(force);

    bool done = is_terminal();
    float reward = done ? 0.0f : 1.0f;

    StepResult result;
    result.next_state = get_state();
    result.reward = reward;
    result.done = done;
    result.info = "sim_step=" + std::to_string(step_count_);
    return result;
}

void CartPoleEnv::update_physics(float force) {
    float cos_theta = std::cos(theta_);
    float sin_theta = std::sin(theta_);
    float total_mass = CART_MASS + POLE_MASS;
    float pole_mass_length = POLE_MASS * POLE_LENGTH;

    float temp = (force + pole_mass_length * theta_dot_ * theta_dot_ * sin_theta) / total_mass;
    float theta_acc = (GRAVITY * sin_theta - cos_theta * temp) /
                      (POLE_LENGTH * (4.0f/3.0f - POLE_MASS * cos_theta * cos_theta / total_mass));
    float x_acc = temp - pole_mass_length * theta_acc * cos_theta / total_mass;

    x_ += TAU * x_dot_;
    x_dot_ += TAU * x_acc;
    theta_ += TAU * theta_dot_;
    theta_dot_ += TAU * theta_acc;
}

bool CartPoleEnv::is_terminal() const {
    return (std::abs(x_) > X_THRESHOLD ||
            std::abs(theta_) > THETA_THRESHOLD ||
            step_count_ >= max_steps_);
}

torch::Tensor CartPoleEnv::get_state() const {
    return torch::tensor({x_, x_dot_, theta_, theta_dot_}, torch::kFloat32);
}

} // namespace environment
