/**
 * @file train_robot.cpp
 * @brief Entrenamiento DQN con ROBOT REAL EV3 (vía UDP)
 *
 * Entrena el DQN usando datos de sensores reales del EV3.
 * Comunicación: Jetson ←UDP→ Laptop Bridge ←USB→ EV3
 *
 * REQUISITOS:
 * - Bridge corriendo en laptop (python bridge.py)
 * - EV3 conectado a laptop por USB
 * - Giroscopio en Puerto 2 del EV3
 *
 * USO:
 *   ./train_robot <laptop_ip> [num_episodes]
 *
 * EJEMPLO:
 *   ./train_robot 192.168.1.100 200
 */

#include <iostream>
#include <torch/torch.h>
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <cmath>

#include "dqn/agent.h"
#include "environment/environment_interface.h"
#include "utils/logger.h"
#include "utils/metrics.h"

// ============================================================================
// SENSOR DATA (igual que en main.cpp)
// ============================================================================

struct SensorData {
    float gyro_angle;
    float gyro_rate;
    int touch_front;
    int touch_side;
    bool valid;

    SensorData() : gyro_angle(0.0f), gyro_rate(0.0f),
                   touch_front(-1), touch_side(-1), valid(false) {}

    torch::Tensor toState() const {
        torch::Tensor state = torch::zeros({4}, torch::kFloat32);
        state[0] = std::tanh(gyro_angle / 90.0f);   // Normalizar ángulo
        state[1] = std::tanh(gyro_rate / 180.0f);   // Normalizar velocidad angular
        state[2] = touch_front >= 0 ? static_cast<float>(touch_front) : 0.0f;
        state[3] = touch_side >= 0 ? static_cast<float>(touch_side) : 0.0f;
        return state;
    }
};

// ============================================================================
// UDP ENVIRONMENT - Comunicación con robot real vía bridge
// ============================================================================

class UDPEnvironment : public environment::EnvironmentInterface {
public:
    UDPEnvironment(const std::string& bridge_ip, int bridge_port = 5000,
                   int max_steps = 100, int timeout_sec = 30)
        : bridge_ip_(bridge_ip), bridge_port_(bridge_port),
          max_steps_(max_steps), timeout_sec_(timeout_sec),
          sock_fd_(-1), current_step_(0) {

        std::cout << "[UDPEnvironment] Conectando a " << bridge_ip << ":" << bridge_port << std::endl;

        // Crear socket UDP
        sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd_ < 0) {
            throw std::runtime_error("No se pudo crear socket UDP");
        }

        // Configurar timeouts
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sock_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        struct timeval tv_recv;
        tv_recv.tv_sec = 0;
        tv_recv.tv_usec = 300000;  // 300ms
        setsockopt(sock_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));

        // Configurar dirección del bridge
        memset(&bridge_addr_, 0, sizeof(bridge_addr_));
        bridge_addr_.sin_family = AF_INET;
        bridge_addr_.sin_port = htons(bridge_port);
        if (inet_pton(AF_INET, bridge_ip.c_str(), &bridge_addr_.sin_addr) <= 0) {
            close(sock_fd_);
            throw std::runtime_error("IP inválida: " + bridge_ip);
        }

        std::cout << "[UDPEnvironment] Conectado exitosamente" << std::endl;
    }

    ~UDPEnvironment() {
        close();
    }

    torch::Tensor reset() override {
        std::cout << "[UDPEnvironment] Reset - Iniciando nuevo episodio" << std::endl;
        current_step_ = 0;
        episode_start_time_ = std::chrono::steady_clock::now();

        // Enviar STOP (acción 0) para detener robot
        sendAction(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Obtener estado inicial
        SensorData sensors = receiveSensors();

        if (!sensors.valid) {
            std::cerr << "[WARNING] No se recibieron sensores válidos en reset, usando ceros" << std::endl;
            return torch::zeros({4}, torch::kFloat32);
        }

        previous_sensors_ = sensors;
        return sensors.toState();
    }

    environment::StepResult step(int64_t action) override {
        current_step_++;

        // Enviar acción al bridge
        sendAction(static_cast<int>(action));

        // Recibir sensores
        SensorData sensors = receiveSensors();

        // Calcular reward
        float reward = computeReward(sensors, action);

        // Determinar si el episodio terminó
        bool done = isEpisodeDone(sensors);

        // Crear resultado
        environment::StepResult result;
        result.next_state = sensors.valid ? sensors.toState() : torch::zeros({4}, torch::kFloat32);
        result.reward = reward;
        result.done = done;
        result.info = "step=" + std::to_string(current_step_);

        previous_sensors_ = sensors;

        return result;
    }

    int64_t state_dim() const override { return 4; }
    int64_t action_dim() const override { return 5; }  // STOP, FORWARD, LEFT, RIGHT, BACKWARD

    void close() override {
        if (sock_fd_ >= 0) {
            // Enviar STOP final
            sendAction(0);
            ::close(sock_fd_);
            sock_fd_ = -1;
        }
    }

private:
    void sendAction(int action) {
        std::string msg = std::to_string(action);
        sendto(sock_fd_, msg.c_str(), msg.length(), 0,
               (struct sockaddr*)&bridge_addr_, sizeof(bridge_addr_));
    }

    SensorData receiveSensors() {
        SensorData data;
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));

        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        ssize_t received = recvfrom(sock_fd_, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&from_addr, &from_len);

        if (received < 0) {
            return data;  // valid = false
        }

        buffer[received] = '\0';
        std::string response(buffer);

        if (parseSensorData(response, data)) {
            data.valid = true;
        }

        return data;
    }

    bool parseSensorData(const std::string& csv, SensorData& data) {
        std::istringstream ss(csv);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() != 4) return false;

        try {
            data.gyro_angle = std::stof(tokens[0]);
            data.gyro_rate = std::stof(tokens[1]);
            data.touch_front = std::stoi(tokens[2]);
            data.touch_side = std::stoi(tokens[3]);
            return true;
        } catch (...) {
            return false;
        }
    }

    float computeReward(const SensorData& sensors, int64_t action) {
        if (!sensors.valid) return -1.0f;

        float reward = 0.0f;

        // Penalización por colisión (máxima prioridad)
        if (sensors.touch_front == 1 || sensors.touch_side == 1) {
            return -10.0f;  // Terminal
        }

        // Recompensa por mantener estabilidad (gyro cerca de 0)
        float gyro_magnitude = std::abs(sensors.gyro_angle);
        if (gyro_magnitude < 15.0f) {
            reward += 1.0f;  // Recompensa por estar estable
        } else if (gyro_magnitude > 45.0f) {
            reward -= 0.5f;  // Penalización por inclinación excesiva
        }

        // Recompensa por acción FORWARD (movimiento proactivo)
        if (action == 1) {  // FORWARD
            reward += 0.5f;
        }

        // Penalización por STOP prolongado (evitar inactividad)
        if (action == 0) {  // STOP
            reward -= 0.1f;
        }

        return reward;
    }

    bool isEpisodeDone(const SensorData& sensors) {
        // Colisión detectada
        if (sensors.valid && (sensors.touch_front == 1 || sensors.touch_side == 1)) {
            std::cout << "[Episode Done] Colisión detectada" << std::endl;
            return true;
        }

        // Máximo de pasos alcanzado
        if (current_step_ >= max_steps_) {
            std::cout << "[Episode Done] Máximo de pasos alcanzado" << std::endl;
            return true;
        }

        // Timeout de tiempo real
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - episode_start_time_).count();
        if (elapsed >= timeout_sec_) {
            std::cout << "[Episode Done] Timeout (" << elapsed << "s)" << std::endl;
            return true;
        }

        // Inclinación extrema (robot cayó)
        if (sensors.valid && std::abs(sensors.gyro_angle) > 60.0f) {
            std::cout << "[Episode Done] Inclinación extrema: " << sensors.gyro_angle << "°" << std::endl;
            return true;
        }

        return false;
    }

    std::string bridge_ip_;
    int bridge_port_;
    int max_steps_;
    int timeout_sec_;
    int sock_fd_;
    struct sockaddr_in bridge_addr_;

    int current_step_;
    std::chrono::steady_clock::time_point episode_start_time_;
    SensorData previous_sensors_;
};

// ============================================================================
// MAIN - Entrenamiento con robot real
// ============================================================================

void print_usage(const char* program) {
    std::cout << "Uso: " << program << " <laptop_ip> [num_episodes]" << std::endl;
    std::cout << std::endl;
    std::cout << "Argumentos:" << std::endl;
    std::cout << "  laptop_ip       IP de la laptop con el bridge" << std::endl;
    std::cout << "  num_episodes    Número de episodios (default: 100)" << std::endl;
    std::cout << std::endl;
    std::cout << "Ejemplo:" << std::endl;
    std::cout << "  " << program << " 192.168.1.100 200" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================================================" << std::endl;
    std::cout << "  DQN Training - ROBOT REAL MODE (EV3 vía UDP)" << std::endl;
    std::cout << "  Entrenamiento con sensores y acciones reales" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    // Parsear argumentos
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string laptop_ip = argv[1];
    int num_episodes = (argc > 2) ? std::atoi(argv[2]) : 100;
    int max_steps_per_episode = 100;

    std::cout << "Configuración:" << std::endl;
    std::cout << "  Laptop IP: " << laptop_ip << std::endl;
    std::cout << "  Episodios: " << num_episodes << std::endl;
    std::cout << "  Max steps por episodio: " << max_steps_per_episode << std::endl;
    std::cout << "=========================================================================" << std::endl;

    // Device
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::cout << "[Device] " << device << std::endl;

    // Crear entorno UDP (conecta con robot real)
    std::cout << "\n[Environment] Creando entorno UDP para robot real..." << std::endl;
    auto env = std::make_unique<UDPEnvironment>(laptop_ip, 5000, max_steps_per_episode, 30);

    // Crear agente DQN
    dqn::Hyperparameters params;
    params.learning_rate = 0.0005f;  // Learning rate más bajo para robot real
    params.gamma = 0.99f;
    params.epsilon_start = 0.5f;     // Epsilon inicial más bajo (menos exploración)
    params.epsilon_end = 0.05f;
    params.epsilon_decay = 0.99f;
    params.batch_size = 32;
    params.buffer_capacity = 5000;
    params.target_update_freq = 5;
    params.hidden_dim1 = 128;
    params.hidden_dim2 = 128;

    std::cout << "[Agent] Creando DQN agent..." << std::endl;
    dqn::DQNAgent agent(env->state_dim(), env->action_dim(), params, device);

    // Logger y métricas
    utils::Logger logger("robot_training.log");
    utils::MetricsTracker metrics;

    std::cout << "\n[Training] Iniciando entrenamiento con robot real..." << std::endl;
    std::cout << "IMPORTANTE: Supervisa el robot durante el entrenamiento" << std::endl;
    std::cout << "            Presiona Ctrl+C para detener de forma segura" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    float best_reward = -1000.0f;

    // Training loop
    for (int episode = 1; episode <= num_episodes; ++episode) {
        std::cout << "\n--- Episodio " << episode << "/" << num_episodes << " ---" << std::endl;

        torch::Tensor state = env->reset();
        float episode_reward = 0.0f;
        float episode_loss = 0.0f;
        int loss_count = 0;
        int step = 0;

        for (step = 0; step < max_steps_per_episode; ++step) {
            // Seleccionar acción
            int64_t action = agent.select_action(state, true);

            // Ejecutar en entorno real
            auto result = env->step(action);

            // Almacenar transición
            agent.store_transition(state, action, result.reward, result.next_state, result.done);

            // Entrenar
            float loss = agent.train_step();
            if (loss >= 0.0f) {
                episode_loss += loss;
                loss_count++;
            }

            state = result.next_state;
            episode_reward += result.reward;

            std::cout << "  Step " << (step + 1) << ": action=" << action
                      << ", reward=" << result.reward
                      << ", total=" << episode_reward << std::endl;

            if (result.done) {
                std::cout << "  Episodio terminado después de " << (step + 1) << " pasos" << std::endl;
                break;
            }

            // Pequeña pausa entre acciones (seguridad)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // Decay epsilon
        agent.decay_epsilon();

        // Update target network
        if (episode % params.target_update_freq == 0) {
            agent.update_target_network();
        }

        // Métricas
        float avg_loss = (loss_count > 0) ? (episode_loss / loss_count) : 0.0f;
        metrics.record_episode(episode_reward);
        if (avg_loss > 0.0f) metrics.record_loss(avg_loss);

        // Log
        logger.log_episode(episode, episode_reward, agent.get_epsilon(), avg_loss);

        std::cout << "Resultado: reward=" << episode_reward
                  << ", epsilon=" << agent.get_epsilon()
                  << ", avg_loss=" << avg_loss << std::endl;

        // Guardar mejor modelo
        if (episode_reward > best_reward) {
            best_reward = episode_reward;
            std::string best_path = "models/dqn_robot_best.pt";
            agent.save(best_path);
            std::cout << "[CHECKPOINT] Nuevo mejor modelo guardado: " << best_path
                      << " (reward=" << best_reward << ")" << std::endl;
        }

        // Guardar checkpoint cada 10 episodios
        if (episode % 10 == 0) {
            std::string checkpoint_path = "models/dqn_robot_checkpoint_" + std::to_string(episode) + ".pt";
            agent.save(checkpoint_path);
            float mean_reward = metrics.get_mean_reward(std::min(10, episode));
            std::cout << "[CHECKPOINT] Episodio " << episode
                      << " | Reward medio (10 eps): " << mean_reward << std::endl;
        }

        // Pausa entre episodios para reposicionar robot manualmente
        std::cout << "\n[PAUSA] Reposiciona el robot si es necesario. Siguiente episodio en 5s..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // Guardar modelo final
    std::string final_path = "models/dqn_robot_final.pt";
    agent.save(final_path);

    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Entrenamiento completado" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Modelos guardados:" << std::endl;
    std::cout << "  - Mejor: models/dqn_robot_best.pt (reward=" << best_reward << ")" << std::endl;
    std::cout << "  - Final: " << final_path << std::endl;
    std::cout << "=========================================================================" << std::endl;

    return 0;
}
