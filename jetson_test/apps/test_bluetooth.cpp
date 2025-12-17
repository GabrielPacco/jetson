/**
 * @file test_bluetooth.cpp
 * @brief Bluetooth Communication Test Utility
 *
 * Tests Bluetooth connection and communication with the Lego robot.
 * Useful for verifying connectivity before running training.
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "communication/bluetooth_manager.h"
#include "communication/protocol.h"

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <robot_address>" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  robot_address    Bluetooth MAC address of robot (e.g., 00:1A:7D:DA:71:13)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " 00:1A:7D:DA:71:13" << std::endl;
    std::cout << std::endl;
    std::cout << "Tip: Find robot MAC address with: hcitool scan" << std::endl;
}

void print_sensor_data(const communication::SensorData& data) {
    std::cout << "  Gyroscope: "
              << "x=" << std::fixed << std::setprecision(3) << data.gyro_x << ", "
              << "y=" << data.gyro_y << ", "
              << "z=" << data.gyro_z << std::endl;
    std::cout << "  Contact sensors: "
              << "front=" << data.contact_front << ", "
              << "side=" << data.contact_side << std::endl;
    std::cout << "  Timestamp: " << data.timestamp_ms << " ms" << std::endl;
}

bool test_connection(communication::BluetoothManager& bt) {
    std::cout << "\n[Test 1] Testing connection..." << std::endl;

    if (!bt.connect()) {
        std::cerr << "[FAIL] Could not connect to robot" << std::endl;
        return false;
    }

    std::cout << "[PASS] Connected successfully" << std::endl;
    return true;
}

bool test_sensor_read(communication::BluetoothManager& bt) {
    std::cout << "\n[Test 2] Testing sensor reading..." << std::endl;

    try {
        communication::SensorData data = bt.read_sensors();
        std::cout << "[PASS] Sensor data received:" << std::endl;
        print_sensor_data(data);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Error reading sensors: " << e.what() << std::endl;
        return false;
    }
}

bool test_command_send(communication::BluetoothManager& bt, uint8_t action, const std::string& action_name) {
    std::cout << "\n[Test] Sending command: " << action_name << "..." << std::endl;

    if (!bt.send_command(action)) {
        std::cerr << "[FAIL] Could not send command" << std::endl;
        return false;
    }

    std::cout << "[PASS] Command sent successfully" << std::endl;

    // Wait for action to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(communication::DEFAULT_ACTION_DURATION_MS + 50));

    // Read sensors after action
    try {
        communication::SensorData data = bt.read_sensors();
        std::cout << "[INFO] Sensor state after action:" << std::endl;
        print_sensor_data(data);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[WARN] Could not read sensors after action: " << e.what() << std::endl;
        return false;
    }
}

bool run_interactive_mode(communication::BluetoothManager& bt) {
    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Interactive Mode" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  w - Move forward" << std::endl;
    std::cout << "  s - Move backward" << std::endl;
    std::cout << "  a - Turn left" << std::endl;
    std::cout << "  d - Turn right" << std::endl;
    std::cout << "  r - Read sensors" << std::endl;
    std::cout << "  q - Quit" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    std::string input;
    while (true) {
        std::cout << "\nEnter command: ";
        std::cin >> input;

        if (input == "q" || input == "quit") {
            break;
        }

        if (input == "w") {
            test_command_send(bt, communication::ACTION_FORWARD, "forward");
        } else if (input == "s") {
            test_command_send(bt, communication::ACTION_BACKWARD, "backward");
        } else if (input == "a") {
            test_command_send(bt, communication::ACTION_LEFT, "left");
        } else if (input == "d") {
            test_command_send(bt, communication::ACTION_RIGHT, "right");
        } else if (input == "r") {
            test_sensor_read(bt);
        } else {
            std::cout << "[ERROR] Unknown command: " << input << std::endl;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================================================" << std::endl;
    std::cout << "  Bluetooth Communication Test" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << std::endl;

    // Parse arguments
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string robot_address = argv[1];

    // =========================================================================
    // Create Bluetooth Manager
    // =========================================================================
    std::cout << "[Setup] Creating Bluetooth manager for: " << robot_address << std::endl;
    communication::BluetoothManager bt(robot_address);

    // =========================================================================
    // Run Tests
    // =========================================================================
    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: Connection
    tests_total++;
    if (test_connection(bt)) {
        tests_passed++;
    } else {
        std::cerr << "\n[ABORT] Cannot proceed without connection" << std::endl;
        return 1;
    }

    // Test 2: Sensor reading
    tests_total++;
    if (test_sensor_read(bt)) {
        tests_passed++;
    }

    // Test 3-6: Command sending
    const struct {
        uint8_t code;
        std::string name;
    } actions[] = {
        {communication::ACTION_FORWARD, "forward"},
        {communication::ACTION_BACKWARD, "backward"},
        {communication::ACTION_LEFT, "left"},
        {communication::ACTION_RIGHT, "right"}
    };

    for (const auto& action : actions) {
        tests_total++;
        if (test_command_send(bt, action.code, action.name)) {
            tests_passed++;
        }
        // Small delay between tests
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // =========================================================================
    // Test Results
    // =========================================================================
    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Test Results" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << " / " << tests_total << std::endl;

    if (tests_passed == tests_total) {
        std::cout << "[SUCCESS] All tests passed!" << std::endl;
    } else {
        std::cout << "[WARNING] Some tests failed" << std::endl;
    }

    // =========================================================================
    // Interactive Mode (optional)
    // =========================================================================
    std::cout << "\nEnter interactive mode? (y/n): ";
    std::string response;
    std::cin >> response;

    if (response == "y" || response == "yes") {
        run_interactive_mode(bt);
    }

    // =========================================================================
    // Cleanup
    // =========================================================================
    bt.disconnect();

    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Test Complete" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    return (tests_passed == tests_total) ? 0 : 1;
}
