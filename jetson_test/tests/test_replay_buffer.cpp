/**
 * @file test_replay_buffer.cpp
 * @brief Unit tests for ReplayBuffer
 */

#include <iostream>
#include <torch/torch.h>
#include "dqn/replay_buffer.h"

int main() {
    std::cout << "[Test] Testing ReplayBuffer..." << std::endl;

    try {
        // Test 1: Buffer creation
        dqn::ReplayBuffer buffer(100);
        std::cout << "[PASS] Buffer created with capacity 100" << std::endl;

        // Test 2: Push transitions
        for (int i = 0; i < 50; ++i) {
            torch::Tensor state = torch::randn({4});
            torch::Tensor next_state = torch::randn({4});
            buffer.push(state, i % 4, 1.0f, next_state, false);
        }

        if (buffer.size() == 50) {
            std::cout << "[PASS] Pushed 50 transitions, buffer size: " << buffer.size() << std::endl;
        } else {
            std::cerr << "[FAIL] Incorrect buffer size: " << buffer.size() << std::endl;
            return 1;
        }

        // Test 3: Sample batch
        if (buffer.can_sample(32)) {
            auto batch = buffer.sample(32);
            if (batch.states.size(0) == 32) {
                std::cout << "[PASS] Sampled batch of 32 transitions" << std::endl;
            } else {
                std::cerr << "[FAIL] Incorrect batch size" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "[FAIL] Cannot sample from buffer" << std::endl;
            return 1;
        }

        // Test 4: Overflow behavior
        for (int i = 0; i < 60; ++i) {
            torch::Tensor state = torch::randn({4});
            torch::Tensor next_state = torch::randn({4});
            buffer.push(state, 0, 1.0f, next_state, false);
        }

        if (buffer.size() == 100) {
            std::cout << "[PASS] Buffer correctly limits to capacity (100)" << std::endl;
        } else {
            std::cerr << "[FAIL] Buffer overflow handling failed: " << buffer.size() << std::endl;
            return 1;
        }

        std::cout << "[SUCCESS] All replay buffer tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Exception: " << e.what() << std::endl;
        return 1;
    }
}
