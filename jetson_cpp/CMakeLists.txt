cmake_minimum_required(VERSION 3.10)
project(Jetson_DQN_EV3)

# C++ Standard (necesario para unique_ptr, etc.)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags de compilación
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -pthread")

# ============================================================================
# MODO 1: Compilación BÁSICA (sin dependencias)
# ============================================================================
# Solo necesita C++ estándar y sockets UDP
# Usa política Random para testing

message(STATUS "Compilando en modo BÁSICO (Random Policy)")
message(STATUS "  - No requiere OpenCV, LibTorch, ni TensorRT")
message(STATUS "  - Solo envío UDP para testing")

add_executable(jetson_dqn main.cpp)

# pthread para threading
target_link_libraries(jetson_dqn pthread)

message(STATUS "Ejecutable: jetson_dqn")
message(STATUS "")
message(STATUS "Para compilar:")
message(STATUS "  mkdir -p build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  make")
message(STATUS "")
message(STATUS "Para ejecutar:")
message(STATUS "  ./jetson_dqn <laptop_ip>")
message(STATUS "  ./jetson_dqn 192.168.1.100")

# ============================================================================
# MODO 2: Con DQN COMPLETO (comentado por ahora)
# ============================================================================
# Descomentar cuando quieras integrar un modelo DQN real

# option(USE_DQN "Build with DQN support (requires LibTorch)" OFF)
#
# if(USE_DQN)
#     message(STATUS "Compilando con soporte DQN (LibTorch)")
#
#     # Buscar LibTorch
#     find_package(Torch REQUIRED)
#     include_directories(${TORCH_INCLUDE_DIRS})
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
#
#     # Opcional: OpenCV para cámara
#     find_package(OpenCV)
#     if(OpenCV_FOUND)
#         include_directories(${OpenCV_INCLUDE_DIRS})
#     endif()
#
#     # Compilar con soporte DQN
#     add_executable(jetson_dqn_full
#         main.cpp
#         dqn_agent.cpp
#     )
#
#     # Linkear librerías
#     set(LIBS pthread ${TORCH_LIBRARIES})
#     if(OpenCV_FOUND)
#         set(LIBS ${LIBS} ${OpenCV_LIBS})
#     endif()
#
#     target_link_libraries(jetson_dqn_full ${LIBS})
# endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
