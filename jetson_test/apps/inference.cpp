/**
 * @file inference.cpp
 * @brief DQN Inference Application for Lego Robot
 *
 * Loads a trained DQN model and runs autonomous navigation on the robot.
 */

#include <iostream>
#include <torch/torch.h>
#include <memory>
#include <string>
#include <csignal>
#include <atomic>

#include "dqn/agent.h"
#include "environment/lego_robot_env.h"
#include "utils/logger.h"

// Global flag for graceful shutdown
std::atomic<bool> running(true);

void signal_handler(int signal) {
    std::cout << "\n[Inference] Received interrupt signal. Shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================================================" << std::endl;
    std::cout << "  DQN Inference - Autonomous Lego Robot Navigation" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << std::endl;

    // Setup signal handler for graceful shutdown (Ctrl+C)
    std::signal(SIGINT, signal_handler);

    // =========================================================================
    // 1. Parse Command Line Arguments
    // =========================================================================
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model_path> [robot_address] [config_file]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Arguments:" << std::endl;
        std::cerr << "  model_path       Path to trained model (.pt file)" << std::endl;
        std::cerr << "  robot_address    Bluetooth MAC address (optional, default from config)" << std::endl;
        std::cerr << "  config_file      Configuration file (optional, default: configs/hyperparameters.yaml)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Example:" << std::endl;
        std::cerr << "  " << argv[0] << " models/dqn_best.pt" << std::endl;
        std::cerr << "  " << argv[0] << " models/dqn_best.pt 00:1A:7D:DA:71:13" << std::endl;
        return 1;
    }

    std::string model_path = argv[1];
    std::string robot_address = (argc > 2) ? argv[2] : "";
    std::string config_file = (argc > 3) ? argv[3] : "configs/hyperparameters.yaml";

    // =========================================================================
    // 2. Initialize Device
    // =========================================================================
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::cout << "[Device] Using device: " << device << std::endl;

    // =========================================================================
    // 3. Load Configuration (for environment settings)
    // =========================================================================
    utils::Logger logger;  // Console-only logging for inference

    // Simple default params if config file doesn't exist
    int max_steps = 1000;  // Longer episodes for autonomous operation
    int timeout_sec = 300;  // 5 minutes
    environment::RewardParams reward_params;

    // Try to load config
    try {
        utils::ConfigParser config(config_file);
        if (robot_address.empty()) {
            robot_address = config.get<std::string>("robot.bluetooth_address", "00:00:00:00:00:00");
        }
        max_steps = config.get<int>("environment.max_steps_per_episode", 1000);
        timeout_sec = config.get<int>("environment.episode_timeout_seconds", 300);
    } catch (...) {
        std::cout << "[Config] Using default parameters" << std::endl;
        if (robot_address.empty()) {
            std::cerr << "[Error] Robot address not specified and config file not found" << std::endl;
            return 1;
        }
    }

    // =========================================================================
    // 4. Create Environment
    // =========================================================================
    std::cout << "[Environment] Connecting to robot: " << robot_address << std::endl;
    auto env = std::make_unique<environment::LegoRobotEnv>(
        robot_address, max_steps, timeout_sec, reward_params
    );

    // =========================================================================
    // 5. Create Agent and Load Model
    // =========================================================================
    dqn::Hyperparameters params;  // Use default parameters
    dqn::DQNAgent agent(env->state_dim(), env->action_dim(), params, device);

    std::cout << "[Agent] Loading model from: " << model_path << std::endl;
    agent.load(model_path);
    agent.eval();  // Set to evaluation mode

    logger.info("Model loaded successfully. Starting autonomous navigation...");

    // =========================================================================
    // 6. Inference Loop
    // =========================================================================
    std::cout << "\n[Inference] Starting autonomous navigation..." << std::endl;
    std::cout << "[Inference] Press Ctrl+C to stop" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    int episode_count = 0;

    while (running) {
        episode_count++;
        std::cout << "\n[Episode " << episode_count << "] Starting new episode..." << std::endl;

        // Reset environment
        torch::Tensor state = env->reset();
        float episode_reward = 0.0f;
        int step_count = 0;

        // Run episode with greedy policy (no exploration)
        while (running) {
            step_count++;

            // Select action (greedy - no epsilon exploration)
            int64_t action;
            {
                torch::NoGradGuard no_grad;  // Disable gradients for faster inference
                action = agent.select_action(state, false);  // training=false -> greedy
            }

            // Log action
            std::cout << "  [Step " << step_count << "] Action: "
                     << communication::action_code_to_name(static_cast<uint8_t>(action))
                     << std::flush;

            // Execute action
            environment::StepResult result = env->step(action);

            // Log result
            std::cout << " | Reward: " << result.reward << std::endl;

            // Update state and accumulate reward
            state = result.next_state;
            episode_reward += result.reward;

            // Check if episode ended
            if (result.done) {
                std::cout << "\n[Episode " << episode_count << "] Episode ended" << std::endl;
                std::cout << "  Total reward: " << episode_reward << std::endl;
                std::cout << "  Total steps: " << step_count << std::endl;
                std::cout << "  Info: " << result.info << std::endl;
                break;
            }

            // Small delay for stability
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Wait a bit before starting next episode
        if (running) {
            std::cout << "\n[Inference] Restarting in 2 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    // =========================================================================
    // 7. Cleanup
    // =========================================================================
    std::cout << "\n[Inference] Shutting down..." << std::endl;
    env->close();

    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Inference Complete" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Total episodes: " << episode_count << std::endl;
    std::cout << "=========================================================================" << std::endl;

    return 0;
}
