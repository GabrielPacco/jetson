#ifndef COMMUNICATION_BLUETOOTH_MANAGER_H
#define COMMUNICATION_BLUETOOTH_MANAGER_H

#include <string>
#include <cstdint>
#include "communication/protocol.h"

namespace communication {

/**
 * @brief Sensor data from robot
 *
 * Processed sensor readings with normalized values
 */
struct SensorData {
    float gyro_x;            // Gyroscope X-axis (-1.0 to 1.0)
    float gyro_y;            // Gyroscope Y-axis (-1.0 to 1.0)
    float gyro_z;            // Gyroscope Z-axis (-1.0 to 1.0)
    int contact_front;       // Front contact sensor (0 or 1)
    int contact_side;        // Side contact sensor (0 or 1)
    uint32_t timestamp_ms;   // Robot timestamp in milliseconds
};

/**
 * @brief Bluetooth RFCOMM Manager for Lego Robot
 *
 * Handles Bluetooth connection and communication with the robot.
 * Uses RFCOMM protocol on Linux (BlueZ stack).
 */
class BluetoothManager {
public:
    /**
     * @brief Construct a new Bluetooth Manager
     *
     * @param device_address Bluetooth MAC address of robot (e.g., "00:1A:7D:DA:71:13")
     */
    explicit BluetoothManager(const std::string& device_address);

    /**
     * @brief Destroy the Bluetooth Manager
     *
     * Automatically disconnects if connected
     */
    ~BluetoothManager();

    /**
     * @brief Connect to the robot
     *
     * @return true if connection successful
     */
    bool connect();

    /**
     * @brief Disconnect from the robot
     */
    void disconnect();

    /**
     * @brief Check if connected to robot
     *
     * @return true if connected
     */
    bool is_connected() const;

    /**
     * @brief Send an action command to the robot
     *
     * @param action Action code (0=forward, 1=backward, 2=left, 3=right)
     * @param duration_ms Duration to execute action (default: 100ms)
     * @return true if command sent successfully
     */
    bool send_command(uint8_t action, uint8_t duration_ms = DEFAULT_ACTION_DURATION_MS);

    /**
     * @brief Read sensor data from the robot
     *
     * Blocks until data is received or timeout occurs.
     *
     * @return SensorData Sensor readings
     * @throws std::runtime_error if read fails or timeout
     */
    SensorData read_sensors();

    /**
     * @brief Flush the input buffer
     *
     * Discards any pending data in the receive buffer.
     */
    void flush_buffer();

    /**
     * @brief Set read timeout
     *
     * @param timeout_ms Timeout in milliseconds
     */
    void set_timeout(int timeout_ms);

private:
    std::string device_address_;     // Bluetooth MAC address
    int socket_fd_;                  // Socket file descriptor
    bool connected_;                 // Connection status
    int timeout_ms_;                 // Read timeout in milliseconds

    static constexpr int RFCOMM_CHANNEL = 1;           // RFCOMM channel number
    static constexpr int DEFAULT_TIMEOUT_MS = 1000;    // Default timeout (1 second)
    static constexpr int MAX_RETRIES = 3;              // Max connection retries
};

} // namespace communication

#endif // COMMUNICATION_BLUETOOTH_MANAGER_H
