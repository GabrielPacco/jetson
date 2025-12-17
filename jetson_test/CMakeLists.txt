cmake_minimum_required(VERSION 3.18)
project(DQN_Lego_Robot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Build Configuration
# ==============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ==============================================================================
# Find Dependencies
# ==============================================================================

# LibTorch (PyTorch C++ API)
list(APPEND CMAKE_PREFIX_PATH "/usr/local/libtorch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

message(STATUS "Found LibTorch: ${TORCH_LIBRARIES}")
message(STATUS "LibTorch include dirs: ${TORCH_INCLUDE_DIRS}")

# Bluetooth library
find_library(BLUETOOTH_LIB bluetooth)
if(NOT BLUETOOTH_LIB)
    message(FATAL_ERROR "Bluetooth library not found. Install with: sudo apt install libbluetooth-dev")
endif()
message(STATUS "Found Bluetooth library: ${BLUETOOTH_LIB}")

# YAML-CPP for configuration parsing
find_package(yaml-cpp REQUIRED)
message(STATUS "Found yaml-cpp: ${YAML_CPP_LIBRARIES}")

# Threads (for Bluetooth and async operations)
find_package(Threads REQUIRED)

# ==============================================================================
# Include Directories
# ==============================================================================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${TORCH_INCLUDE_DIRS}
)

# ==============================================================================
# Source Files
# ==============================================================================

# DQN core sources
set(DQN_SOURCES
    src/dqn/network.cpp
    src/dqn/replay_buffer.cpp
    src/dqn/agent.cpp
)

# Environment sources
set(ENV_SOURCES
    src/environment/environment_interface.cpp
    src/environment/lego_robot_env.cpp
    src/environment/cartpole_env.cpp
)

# Communication sources
set(COMM_SOURCES
    src/communication/bluetooth_manager.cpp
    src/communication/protocol.cpp
)

# Utility sources
set(UTIL_SOURCES
    src/utils/logger.cpp
    src/utils/metrics.cpp
    src/utils/config_parser.cpp
)

# ==============================================================================
# Create DQN Library
# ==============================================================================
add_library(dqn_lib STATIC
    ${DQN_SOURCES}
    ${ENV_SOURCES}
    ${COMM_SOURCES}
    ${UTIL_SOURCES}
)

target_link_libraries(dqn_lib
    ${TORCH_LIBRARIES}
    ${BLUETOOTH_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
    yaml-cpp
)

# ==============================================================================
# Executables
# ==============================================================================

# Training application (con robot físico)
add_executable(train apps/train.cpp)
target_link_libraries(train dqn_lib)

# Training en SIMULACIÓN (sin robot físico - solo DQN)
add_executable(train_simulation apps/train_simulation.cpp)
target_link_libraries(train_simulation dqn_lib)

# Inference application
add_executable(inference apps/inference.cpp)
target_link_libraries(inference dqn_lib)

# Bluetooth test utility
add_executable(test_bluetooth apps/test_bluetooth.cpp)
target_link_libraries(test_bluetooth dqn_lib)

# ==============================================================================
# Tests
# ==============================================================================
enable_testing()

# Network test
add_executable(test_network tests/test_network.cpp)
target_link_libraries(test_network dqn_lib)
add_test(NAME NetworkTest COMMAND test_network)

# Replay buffer test
add_executable(test_replay_buffer tests/test_replay_buffer.cpp)
target_link_libraries(test_replay_buffer dqn_lib)
add_test(NAME ReplayBufferTest COMMAND test_replay_buffer)

# Environment test
add_executable(test_environment tests/test_environment.cpp)
target_link_libraries(test_environment dqn_lib)
add_test(NAME EnvironmentTest COMMAND test_environment)

# ==============================================================================
# Installation
# ==============================================================================
install(TARGETS train train_simulation inference test_bluetooth
    RUNTIME DESTINATION bin
)

install(TARGETS dqn_lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# ==============================================================================
# Print Configuration Summary
# ==============================================================================
message(STATUS "========================================")
message(STATUS "DQN Lego Robot Configuration Summary")
message(STATUS "========================================")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  LibTorch: ${TORCH_LIBRARIES}")
message(STATUS "  Bluetooth: ${BLUETOOTH_LIB}")
message(STATUS "  YAML-CPP: ${YAML_CPP_LIBRARIES}")
message(STATUS "========================================")
