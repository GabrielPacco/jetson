#include "communication/protocol.h"
#include <string>
#include <algorithm>

namespace communication {

uint8_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint8_t checksum = 0;

    for (size_t i = 0; i < size; ++i) {
        checksum ^= bytes[i];  // XOR all bytes
    }

    return checksum;
}

bool verify_checksum(const void* data, size_t size, uint8_t checksum) {
    uint8_t calculated = calculate_checksum(data, size);
    return calculated == checksum;
}

CommandPacket create_command_packet(uint8_t action, uint8_t duration_ms) {
    CommandPacket packet;
    packet.header = COMMAND_HEADER;
    packet.action = action;
    packet.duration_ms = duration_ms;

    // Calculate checksum of all bytes except checksum field
    packet.checksum = calculate_checksum(&packet, sizeof(packet) - 1);

    return packet;
}

bool parse_sensor_packet(const uint8_t* data, size_t size, SensorPacket& packet) {
    // Check minimum size
    if (size < sizeof(SensorPacket)) {
        return false;
    }

    // Copy data to packet structure
    std::memcpy(&packet, data, sizeof(SensorPacket));

    // Verify header
    if (packet.header != SENSOR_HEADER) {
        return false;
    }

    // Verify checksum
    if (!verify_checksum(&packet, sizeof(SensorPacket) - 1, packet.checksum)) {
        return false;
    }

    return true;
}

uint8_t action_name_to_code(const std::string& action_name) {
    std::string lower = action_name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "forward") return ACTION_FORWARD;
    if (lower == "backward") return ACTION_BACKWARD;
    if (lower == "left") return ACTION_LEFT;
    if (lower == "right") return ACTION_RIGHT;

    return 0xFF;  // Invalid action
}

std::string action_code_to_name(uint8_t action_code) {
    switch (action_code) {
        case ACTION_FORWARD:  return "forward";
        case ACTION_BACKWARD: return "backward";
        case ACTION_LEFT:     return "left";
        case ACTION_RIGHT:    return "right";
        default:              return "unknown";
    }
}

} // namespace communication
