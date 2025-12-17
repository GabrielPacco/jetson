#ifndef COMMUNICATION_PROTOCOL_H
#define COMMUNICATION_PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <string>

namespace communication {

// ==============================================================================
// Protocol Constants
// ==============================================================================

constexpr uint8_t COMMAND_HEADER = 0xAA;       // Start marker for command packets
constexpr uint8_t SENSOR_HEADER = 0xBB;        // Start marker for sensor packets
constexpr uint8_t DEFAULT_ACTION_DURATION_MS = 100;  // Default action duration

// Action codes
constexpr uint8_t ACTION_FORWARD = 0;
constexpr uint8_t ACTION_BACKWARD = 1;
constexpr uint8_t ACTION_LEFT = 2;
constexpr uint8_t ACTION_RIGHT = 3;

// ==============================================================================
// Packet Structures
// ==============================================================================

/**
 * @brief Command packet sent from Jetson to Robot
 *
 * Total size: 4 bytes
 */
#pragma pack(push, 1)
struct CommandPacket {
    uint8_t header;          // 0xAA - start marker
    uint8_t action;          // Action code (0=forward, 1=backward, 2=left, 3=right)
    uint8_t duration_ms;     // How long to execute action (0-255 ms)
    uint8_t checksum;        // XOR checksum of previous bytes
};
#pragma pack(pop)

/**
 * @brief Sensor packet received from Robot to Jetson
 *
 * Total size: 14 bytes
 */
#pragma pack(push, 1)
struct SensorPacket {
    uint8_t header;          // 0xBB - start marker
    int16_t gyro_x;          // Gyroscope X-axis (raw ADC value, -32768 to 32767)
    int16_t gyro_y;          // Gyroscope Y-axis (raw ADC value, -32768 to 32767)
    int16_t gyro_z;          // Gyroscope Z-axis (raw ADC value, -32768 to 32767)
    uint8_t contact_front;   // Front contact sensor (0 or 1)
    uint8_t contact_side;    // Side contact sensor (0 or 1)
    uint32_t timestamp;      // Robot timestamp in milliseconds
    uint8_t checksum;        // XOR checksum of previous bytes
};
#pragma pack(pop)

// ==============================================================================
// Helper Functions
// ==============================================================================

/**
 * @brief Calculate XOR checksum for data
 *
 * @param data Pointer to data buffer
 * @param size Number of bytes to checksum
 * @return uint8_t Checksum value
 */
uint8_t calculate_checksum(const void* data, size_t size);

/**
 * @brief Verify checksum of data
 *
 * @param data Pointer to data buffer
 * @param size Number of bytes to verify (excluding checksum byte)
 * @param checksum Expected checksum value
 * @return true if checksum is valid
 */
bool verify_checksum(const void* data, size_t size, uint8_t checksum);

/**
 * @brief Create a command packet
 *
 * @param action Action code
 * @param duration_ms Action duration in milliseconds
 * @return CommandPacket Initialized packet with checksum
 */
CommandPacket create_command_packet(uint8_t action, uint8_t duration_ms = DEFAULT_ACTION_DURATION_MS);

/**
 * @brief Parse and validate a sensor packet
 *
 * @param data Raw data buffer
 * @param size Size of buffer
 * @param packet Output sensor packet
 * @return true if packet is valid and parsed successfully
 */
bool parse_sensor_packet(const uint8_t* data, size_t size, SensorPacket& packet);

/**
 * @brief Convert action name to action code
 *
 * @param action_name String name ("forward", "backward", "left", "right")
 * @return uint8_t Action code (or 0xFF if invalid)
 */
uint8_t action_name_to_code(const std::string& action_name);

/**
 * @brief Convert action code to action name
 *
 * @param action_code Action code
 * @return std::string Action name
 */
std::string action_code_to_name(uint8_t action_code);

} // namespace communication

#endif // COMMUNICATION_PROTOCOL_H
