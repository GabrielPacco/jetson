/**
 * @file test_network.cpp
 * @brief Unit tests for QNetwork
 */

#include <iostream>
#include <torch/torch.h>
#include "dqn/network.h"

int main() {
    std::cout << "[Test] Testing QNetwork..." << std::endl;

    try {
        // Test 1: Network creation
        dqn::QNetwork network(4, 4, 128, 128);
        std::cout << "[PASS] Network created successfully" << std::endl;

        // Test 2: Forward pass
        torch::Tensor input = torch::randn({1, 4});
        torch::Tensor output = network->forward(input);

        if (output.sizes() == torch::IntArrayRef({1, 4})) {
            std::cout << "[PASS] Forward pass - correct output shape" << std::endl;
        } else {
            std::cerr << "[FAIL] Forward pass - incorrect output shape" << std::endl;
            return 1;
        }

        // Test 3: Batch forward pass
        torch::Tensor batch_input = torch::randn({32, 4});
        torch::Tensor batch_output = network->forward(batch_input);

        if (batch_output.sizes() == torch::IntArrayRef({32, 4})) {
            std::cout << "[PASS] Batch forward pass - correct output shape" << std::endl;
        } else {
            std::cerr << "[FAIL] Batch forward pass - incorrect output shape" << std::endl;
            return 1;
        }

        std::cout << "[SUCCESS] All network tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Exception: " << e.what() << std::endl;
        return 1;
    }
}
