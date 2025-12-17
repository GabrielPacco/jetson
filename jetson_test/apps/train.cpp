/**
 * @file train.cpp
 * @brief DQN Training Application for Lego Robot
 *
 * Trains a DQN agent to navigate and avoid obstacles using the Lego robot.
 */

#include <iostream>
#include <torch/torch.h>
#include <memory>
#include <string>
#include <filesystem>

#include "dqn/agent.h"
#include "environment/lego_robot_env.h"
#include "utils/logger.h"
#include "utils/metrics.h"
#include "utils/config_parser.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::cout << "=========================================================================" << std::endl;
    std::cout << "  DQN Training - Lego Robot Navigation with Obstacle Avoidance" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 1. Load Configuration
    // =========================================================================
    std::string config_file = "configs/hyperparameters.yaml";
    if (argc > 1) {
        config_file = argv[1];
    }

    std::cout << "[Config] Loading configuration from: " << config_file << std::endl;
    utils::ConfigParser config(config_file);

    // =========================================================================
    // 2. Initialize Device (CUDA if available)
    // =========================================================================
    bool use_cuda = config.get<bool>("device.use_cuda", true);
    torch::Device device(use_cuda && torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    std::cout << "[Device] Using device: " << device << std::endl;
    if (device.is_cuda()) {
        std::cout << "[Device] CUDA available: " << torch::cuda::device_count() << " GPU(s)" << std::endl;
    }

    // =========================================================================
    // 3. Create Environment
    // =========================================================================
    std::string robot_address = config.get<std::string>("robot.bluetooth_address", "00:00:00:00:00:00");
    int max_steps = config.get<int>("environment.max_steps_per_episode", 200);
    int timeout_sec = config.get<int>("environment.episode_timeout_seconds", 60);

    environment::RewardParams reward_params;
    reward_params.forward_success = config.get<float>("reward.forward_success", 1.0f);
    reward_params.collision_penalty = config.get<float>("reward.collision_penalty", -1.0f);
    reward_params.backward_penalty = config.get<float>("reward.backward_penalty", -0.1f);
    reward_params.turn_reward = config.get<float>("reward.turn_reward", 0.0f);
    reward_params.orientation_bonus = config.get<float>("reward.orientation_bonus", 0.5f);

    std::cout << "[Environment] Creating Lego Robot environment..." << std::endl;
    auto env = std::make_unique<environment::LegoRobotEnv>(
        robot_address, max_steps, timeout_sec, reward_params
    );

    // =========================================================================
    // 4. Create DQN Agent
    // =========================================================================
    dqn::Hyperparameters params;
    params.learning_rate = config.get<float>("training.learning_rate", 0.001f);
    params.gamma = config.get<float>("training.gamma", 0.99f);
    params.epsilon_start = config.get<float>("training.epsilon_start", 1.0f);
    params.epsilon_end = config.get<float>("training.epsilon_end", 0.05f);
    params.epsilon_decay = config.get<float>("training.epsilon_decay", 0.995f);
    params.batch_size = config.get<int>("replay.batch_size", 64);
    params.buffer_capacity = config.get<int>("replay.capacity", 10000);
    params.target_update_freq = config.get<int>("target.update_frequency", 10);
    params.hidden_dim1 = config.get<int>("network.hidden_dim1", 128);
    params.hidden_dim2 = config.get<int>("network.hidden_dim2", 128);

    std::cout << "[Agent] Creating DQN agent..." << std::endl;
    dqn::DQNAgent agent(env->state_dim(), env->action_dim(), params, device);

    // =========================================================================
    // 5. Setup Logging and Metrics
    // =========================================================================
    std::string log_file = config.get<std::string>("logging.log_file", "training.log");
    utils::Logger logger(log_file);
    utils::MetricsTracker metrics;

    // =========================================================================
    // 6. Training Loop
    // =========================================================================
    int num_episodes = config.get<int>("training.num_episodes", 500);
    int log_interval = config.get<int>("logging.log_interval", 10);
    int checkpoint_interval = config.get<int>("logging.checkpoint_interval", 50);
    std::string best_model_path = config.get<std::string>("paths.best_model", "models/dqn_best.pt");
    std::string final_model_path = config.get<std::string>("paths.final_model", "models/dqn_final.pt");

    // Create models directory if it doesn't exist
    fs::create_directories("models");

    std::cout << "\n[Training] Starting training for " << num_episodes << " episodes..." << std::endl;
    logger.info("Training started");

    for (int episode = 1; episode <= num_episodes; ++episode) {
        // Reset environment
        torch::Tensor state = env->reset();
        float episode_reward = 0.0f;
        float episode_loss_sum = 0.0f;
        int loss_count = 0;

        // Run episode
        for (int step = 0; step < max_steps; ++step) {
            // Select action (epsilon-greedy)
            int64_t action = agent.select_action(state, true);

            // Execute action in environment
            environment::StepResult result = env->step(action);

            // Store transition
            agent.store_transition(state, action, result.reward, result.next_state, result.done);

            // Train agent
            float loss = agent.train_step();
            if (loss >= 0.0f) {
                episode_loss_sum += loss;
                loss_count++;
            }

            // Update state and accumulate reward
            state = result.next_state;
            episode_reward += result.reward;

            if (result.done) {
                break;
            }
        }

        // Decay epsilon
        agent.decay_epsilon();

        // Update target network periodically
        if (episode % params.target_update_freq == 0) {
            agent.update_target_network();
        }

        // Record metrics
        float avg_loss = (loss_count > 0) ? (episode_loss_sum / loss_count) : -1.0f;
        metrics.record_episode(episode_reward);
        if (avg_loss >= 0.0f) {
            metrics.record_loss(avg_loss);
        }

        // Logging
        if (episode % log_interval == 0) {
            float mean_reward = metrics.get_mean_reward(100);
            logger.log_episode(episode, episode_reward, agent.get_epsilon(), avg_loss);
            std::cout << "  Mean reward (100 eps): " << mean_reward << std::endl;
        }

        // Save best model
        if (metrics.is_best_reward(episode_reward)) {
            agent.save(best_model_path);
            logger.info("New best model saved with reward: " + std::to_string(episode_reward));
        }

        // Periodic checkpoint
        if (episode % checkpoint_interval == 0) {
            std::string checkpoint_path = "models/dqn_checkpoint_" + std::to_string(episode) + ".pt";
            agent.save(checkpoint_path);
            logger.info("Checkpoint saved at episode " + std::to_string(episode));
        }
    }

    // =========================================================================
    // 7. Save Final Results
    // =========================================================================
    agent.save(final_model_path);
    logger.info("Final model saved");

    std::string metrics_file = config.get<std::string>("logging.metrics_file", "training_metrics.csv");
    metrics.save_to_file(metrics_file);

    // =========================================================================
    // 8. Cleanup
    // =========================================================================
    env->close();

    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Training Complete!" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Total episodes: " << num_episodes << std::endl;
    std::cout << "Best reward: " << metrics.get_best_reward() << std::endl;
    std::cout << "Final epsilon: " << agent.get_epsilon() << std::endl;
    std::cout << "\nSaved files:" << std::endl;
    std::cout << "  - Best model: " << best_model_path << std::endl;
    std::cout << "  - Final model: " << final_model_path << std::endl;
    std::cout << "  - Metrics: " << metrics_file << std::endl;
    std::cout << "  - Log: " << log_file << std::endl;
    std::cout << "=========================================================================" << std::endl;

    return 0;
}
