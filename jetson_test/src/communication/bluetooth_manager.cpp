#include "communication/bluetooth_manager.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

// Bluetooth headers (BlueZ on Linux)
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

namespace communication {

BluetoothManager::BluetoothManager(const std::string& device_address)
    : device_address_(device_address),
      socket_fd_(-1),
      connected_(false),
      timeout_ms_(DEFAULT_TIMEOUT_MS) {
}

BluetoothManager::~BluetoothManager() {
    disconnect();
}

bool BluetoothManager::connect() {
    if (connected_) {
        std::cout << "[BluetoothManager] Already connected" << std::endl;
        return true;
    }

    std::cout << "[BluetoothManager] Connecting to " << device_address_ << "..." << std::endl;

    // Create RFCOMM socket
    socket_fd_ = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (socket_fd_ < 0) {
        std::cerr << "[BluetoothManager] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set up connection parameters
    struct sockaddr_rc addr = {0};
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t)RFCOMM_CHANNEL;

    // Convert MAC address string to bdaddr_t
    if (str2ba(device_address_.c_str(), &addr.rc_bdaddr) < 0) {
        std::cerr << "[BluetoothManager] Invalid Bluetooth address: " << device_address_ << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Connect to robot with retries
    int retries = 0;
    while (retries < MAX_RETRIES) {
        int status = ::connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr));

        if (status == 0) {
            // Connection successful
            connected_ = true;
            std::cout << "[BluetoothManager] Connected successfully!" << std::endl;

            // Set socket timeout
            struct timeval tv;
            tv.tv_sec = timeout_ms_ / 1000;
            tv.tv_usec = (timeout_ms_ % 1000) * 1000;
            setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            // Flush any initial data
            flush_buffer();

            return true;
        }

        retries++;
        std::cerr << "[BluetoothManager] Connection attempt " << retries
                  << " failed: " << strerror(errno) << std::endl;

        if (retries < MAX_RETRIES) {
            std::cout << "[BluetoothManager] Retrying in 1 second..." << std::endl;
            sleep(1);
        }
    }

    // All retries failed
    std::cerr << "[BluetoothManager] Failed to connect after " << MAX_RETRIES << " attempts" << std::endl;
    close(socket_fd_);
    socket_fd_ = -1;
    return false;
}

void BluetoothManager::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        std::cout << "[BluetoothManager] Disconnected" << std::endl;
    }
}

bool BluetoothManager::is_connected() const {
    return connected_;
}

bool BluetoothManager::send_command(uint8_t action, uint8_t duration_ms) {
    if (!connected_) {
        std::cerr << "[BluetoothManager] Not connected to robot" << std::endl;
        return false;
    }

    // Create command packet
    CommandPacket packet = create_command_packet(action, duration_ms);

    // Send packet
    ssize_t written = write(socket_fd_, &packet, sizeof(packet));

    if (written != sizeof(packet)) {
        std::cerr << "[BluetoothManager] Failed to send command: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

SensorData BluetoothManager::read_sensors() {
    if (!connected_) {
        throw std::runtime_error("BluetoothManager: Not connected to robot");
    }

    // Read sensor packet
    uint8_t buffer[sizeof(SensorPacket)];
    ssize_t bytes_read = read(socket_fd_, buffer, sizeof(buffer));

    if (bytes_read < 0) {
        throw std::runtime_error("BluetoothManager: Read failed - " + std::string(strerror(errno)));
    }

    if (bytes_read == 0) {
        throw std::runtime_error("BluetoothManager: Connection closed by robot");
    }

    if (bytes_read != sizeof(SensorPacket)) {
        throw std::runtime_error("BluetoothManager: Incomplete packet received (got " +
                                std::to_string(bytes_read) + " bytes, expected " +
                                std::to_string(sizeof(SensorPacket)) + ")");
    }

    // Parse sensor packet
    SensorPacket packet;
    if (!parse_sensor_packet(buffer, bytes_read, packet)) {
        throw std::runtime_error("BluetoothManager: Invalid sensor packet (bad header or checksum)");
    }

    // Convert to SensorData with normalized values
    SensorData data;

    // Normalize gyroscope values from int16_t (-32768 to 32767) to float (-1.0 to 1.0)
    data.gyro_x = packet.gyro_x / 32768.0f;
    data.gyro_y = packet.gyro_y / 32768.0f;
    data.gyro_z = packet.gyro_z / 32768.0f;

    // Contact sensors (already 0 or 1)
    data.contact_front = packet.contact_front;
    data.contact_side = packet.contact_side;

    // Timestamp
    data.timestamp_ms = packet.timestamp;

    return data;
}

void BluetoothManager::flush_buffer() {
    if (!connected_) {
        return;
    }

    // Set socket to non-blocking mode temporarily
    struct timeval tv = {0, 0};
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Read and discard all available data
    uint8_t buffer[256];
    while (read(socket_fd_, buffer, sizeof(buffer)) > 0) {
        // Keep reading until nothing left
    }

    // Restore timeout
    tv.tv_sec = timeout_ms_ / 1000;
    tv.tv_usec = (timeout_ms_ % 1000) * 1000;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void BluetoothManager::set_timeout(int timeout_ms) {
    timeout_ms_ = timeout_ms;

    if (connected_ && socket_fd_ >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout_ms_ / 1000;
        tv.tv_usec = (timeout_ms_ % 1000) * 1000;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
}

} // namespace communication
