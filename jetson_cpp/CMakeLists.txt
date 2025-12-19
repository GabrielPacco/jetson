cmake_minimum_required(VERSION 3.18)
project(Jetson_DQN_EV3 LANGUAGES CXX)

# C++ Standard (C++17 requerido por LibTorch)
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
# Find LibTorch (REQUERIDO - ya instalado en Jetson)
# ==============================================================================
list(APPEND CMAKE_PREFIX_PATH "/usr/local/libtorch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

message(STATUS "Found LibTorch: ${TORCH_LIBRARIES}")
message(STATUS "LibTorch include dirs: ${TORCH_INCLUDE_DIRS}")

# Threads (para comunicación UDP)
find_package(Threads REQUIRED)

# ==============================================================================
# Include Directories
# ==============================================================================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${TORCH_INCLUDE_DIRS}
)

# ==============================================================================
# DQN Library (código probado de jetson_test)
# ==============================================================================
add_library(dqn_core STATIC
    src/dqn/network.cpp
    src/dqn/replay_buffer.cpp
    src/dqn/agent.cpp
    src/environment/environment_interface.cpp
    src/environment/cartpole_env.cpp
    src/utils/logger.cpp
    src/utils/metrics.cpp
    # NOTA: config_parser.cpp NO se incluye porque requiere yaml-cpp
    # train_simulation usa parámetros hardcodeados (no necesita yaml)
)

target_link_libraries(dqn_core
    ${TORCH_LIBRARIES}
)

# ==============================================================================
# Executables
# ==============================================================================

# Inference executable (DQN + UDP)
add_executable(jetson_dqn main.cpp)
target_link_libraries(jetson_dqn
    dqn_core
    ${CMAKE_THREAD_LIBS_INIT}
)

# Training executable (Simulation mode - NO requiere robot)
add_executable(train_simulation apps/train_simulation.cpp)
target_link_libraries(train_simulation dqn_core)

# Training executable (Robot real mode - Requiere EV3 + bridge)
add_executable(train_robot apps/train_robot.cpp)
target_link_libraries(train_robot
    dqn_core
    ${CMAKE_THREAD_LIBS_INIT}
)

# ==============================================================================
# Print Configuration Summary
# ==============================================================================
message(STATUS "========================================")
message(STATUS "Jetson DQN + UDP Configuration")
message(STATUS "========================================")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  LibTorch: ${TORCH_LIBRARIES}")
message(STATUS "========================================")
message(STATUS "")
message(STATUS "Para compilar:")
message(STATUS "  cd jetson_cpp")
message(STATUS "  mkdir -p build && cd build")
message(STATUS "  cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch ..")
message(STATUS "  make -j4")
message(STATUS "")
message(STATUS "Para entrenar en SIMULACIÓN (sin robot):")
message(STATUS "  ./train_simulation [num_episodes]")
message(STATUS "")
message(STATUS "Para entrenar con ROBOT REAL (requiere bridge + EV3):")
message(STATUS "  ./train_robot <laptop_ip> [num_episodes]")
message(STATUS "")
message(STATUS "Para ejecutar inferencia:")
message(STATUS "  ./jetson_dqn <laptop_ip> -p dqn [-m modelo.pt]")
message(STATUS "========================================")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
