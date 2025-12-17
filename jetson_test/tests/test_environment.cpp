/**
 * @file test_environment.cpp
 * @brief Unit tests for environment interface
 */

#include <iostream>
#include "communication/protocol.h"

int main() {
    std::cout << "[Test] Testing Protocol and Communication..." << std::endl;

    try {
        // Test 1: Command packet creation
        auto cmd = communication::create_command_packet(communication::ACTION_FORWARD, 100);

        if (cmd.header == communication::COMMAND_HEADER &&
            cmd.action == communication::ACTION_FORWARD) {
            std::cout << "[PASS] Command packet created correctly" << std::endl;
        } else {
            std::cerr << "[FAIL] Command packet creation failed" << std::endl;
            return 1;
        }

        // Test 2: Checksum calculation
        uint8_t test_data[] = {0xAA, 0x01, 0x64};
        uint8_t checksum = communication::calculate_checksum(test_data, 3);

        if (communication::verify_checksum(test_data, 3, checksum)) {
            std::cout << "[PASS] Checksum calculation and verification" << std::endl;
        } else {
            std::cerr << "[FAIL] Checksum verification failed" << std::endl;
            return 1;
        }

        // Test 3: Action code conversion
        if (communication::action_name_to_code("forward") == communication::ACTION_FORWARD &&
            communication::action_name_to_code("backward") == communication::ACTION_BACKWARD &&
            communication::action_name_to_code("left") == communication::ACTION_LEFT &&
            communication::action_name_to_code("right") == communication::ACTION_RIGHT) {
            std::cout << "[PASS] Action name to code conversion" << std::endl;
        } else {
            std::cerr << "[FAIL] Action conversion failed" << std::endl;
            return 1;
        }

        // Test 4: Code to name conversion
        if (communication::action_code_to_name(communication::ACTION_FORWARD) == "forward" &&
            communication::action_code_to_name(communication::ACTION_BACKWARD) == "backward") {
            std::cout << "[PASS] Action code to name conversion" << std::endl;
        } else {
            std::cerr << "[FAIL] Code to name conversion failed" << std::endl;
            return 1;
        }

        std::cout << "[SUCCESS] All protocol tests passed!" << std::endl;
        std::cout << "[NOTE] Full environment test requires connected robot" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Exception: " << e.what() << std::endl;
        return 1;
    }
}
