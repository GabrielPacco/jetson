/**
 * main.cpp
 *
 * DQN Inference para EV3 - Jetson → Laptop Bridge → EV3
 * Versión adaptada del repositorio jetson_test
 *
 * OPCIONES:
 * 1. Modo DQN: Carga modelo entrenado y ejecuta inferencia
 * 2. Modo Random: Acciones aleatorias para testing
 *
 * Compilar:
 *   mkdir build && cd build
 *   cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch ..
 *   make -j4
 *
 * Ejecutar:
 *   ./jetson_dqn <laptop_ip> -p random                    # Modo random
 *   ./jetson_dqn <laptop_ip> -p dqn                       # Modo DQN (sin modelo)
 *   ./jetson_dqn <laptop_ip> -p dqn -m models/dqn.pt     # Modo DQN con modelo
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <random>
#include <memory>

// DQN includes (código probado de jetson_test)
#include <torch/torch.h>
#include "dqn/agent.h"
#include "dqn/types.h"

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

// Puerto UDP del bridge (debe coincidir con config.py)
const int UDP_PORT = 5000;

// Frecuencia de acciones (Hz)
const int ACTION_FREQUENCY = 5;

// Timeout para conexión UDP (segundos)
const int UDP_TIMEOUT = 5;

// Nombres de acciones (para logging)
const char* ACTION_NAMES[] = {
    "STOP",        // 0
    "FORWARD",     // 1
    "TURN_LEFT",   // 2
    "TURN_RIGHT",  // 3
    "BACKWARD"     // 4
};

// Número de acciones disponibles
const int NUM_ACTIONS = 5;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

volatile sig_atomic_t running = 1;

void signalHandler(int signum) {
    std::cout << "\n[INFO] Interrupción recibida (Ctrl+C), deteniendo..." << std::endl;
    running = 0;
}

// ============================================================================
// CLASE UDP SENDER
// ============================================================================

/**
 * Envía acciones por UDP al bridge de la laptop
 */
class UDPSender {
public:
    UDPSender(const std::string& ip, int port)
        : laptop_ip_(ip), port_(port), sock_fd_(-1), connected_(false) {

        // Crear socket UDP
        sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd_ < 0) {
            std::cerr << "[ERROR] No se pudo crear socket UDP: "
                      << strerror(errno) << std::endl;
            return;
        }

        // Configurar timeout para send
        struct timeval tv;
        tv.tv_sec = UDP_TIMEOUT;
        tv.tv_usec = 0;
        setsockopt(sock_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        // Configurar dirección del servidor
        memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &server_addr_.sin_addr) <= 0) {
            std::cerr << "[ERROR] Dirección IP inválida: " << ip << std::endl;
            close(sock_fd_);
            sock_fd_ = -1;
            return;
        }

        connected_ = true;
        std::cout << "[UDP] Listo para enviar a " << ip << ":" << port << std::endl;
    }

    ~UDPSender() {
        if (sock_fd_ >= 0) {
            // Enviar STOP final antes de cerrar
            send(0);
            close(sock_fd_);
        }
    }

    bool send(int action) {
        if (!connected_ || sock_fd_ < 0) {
            std::cerr << "[ERROR] Socket UDP no inicializado" << std::endl;
            return false;
        }

        if (action < 0 || action >= NUM_ACTIONS) {
            std::cerr << "[ERROR] Acción inválida: " << action << std::endl;
            return false;
        }

        // Convertir acción a string
        std::string msg = std::to_string(action);

        // Enviar por UDP
        ssize_t sent = sendto(sock_fd_, msg.c_str(), msg.length(), 0,
                              (struct sockaddr*)&server_addr_, sizeof(server_addr_));

        if (sent < 0) {
            std::cerr << "[ERROR] Error al enviar: " << strerror(errno) << std::endl;
            return false;
        }

        // Log con timestamp
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        auto timer = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_info = std::localtime(&timer);

        char buffer[64];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", tm_info);

        std::cout << "[" << buffer << "." << ms.count() << "] "
                  << "Action: " << action << " (" << ACTION_NAMES[action] << ")"
                  << std::endl;

        return true;
    }

    bool isConnected() const { return connected_; }

private:
    int sock_fd_;
    struct sockaddr_in server_addr_;
    std::string laptop_ip_;
    int port_;
    bool connected_;
};

// ============================================================================
// POLICY - Selección de Acción
// ============================================================================

/**
 * Policy base para selección de acciones
 */
class Policy {
public:
    virtual ~Policy() = default;
    virtual int selectAction() = 0;
    virtual std::string getName() const = 0;
};

/**
 * Política aleatoria (para testing sin DQN)
 */
class RandomPolicy : public Policy {
public:
    RandomPolicy()
        : rng_(std::random_device{}())
        , dist_(0, NUM_ACTIONS - 1) {}

    int selectAction() override {
        return dist_(rng_);
    }

    std::string getName() const override {
        return "Random";
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
};

/**
 * Política DQN (usa red neuronal entrenada)
 * Código DQN probado y verificado de jetson_test
 */
class DQNPolicy : public Policy {
public:
    DQNPolicy(const std::string& model_path = "")
        : model_loaded_(false) {

        // Configurar device (CUDA si está disponible)
        device_ = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
        std::cout << "[DQNPolicy] Using device: " << device_ << std::endl;

        // Parámetros del entorno EV3
        // state_dim = 4: [gyro_x, gyro_y, contact_front, contact_side]
        // action_dim = 5: [STOP, FORWARD, LEFT, RIGHT, BACKWARD]
        const int64_t state_dim = 4;
        const int64_t action_dim = NUM_ACTIONS;

        // Inicializar hiperparámetros (usar defaults)
        dqn::Hyperparameters params;

        // Crear agente DQN (código probado de jetson_test)
        agent_ = std::make_unique<dqn::DQNAgent>(state_dim, action_dim, params, device_);
        agent_->eval();  // Modo evaluación (no entrenamiento)

        // Cargar modelo si se especificó
        if (!model_path.empty()) {
            loadModel(model_path);
        } else {
            std::cout << "[DQNPolicy] No model specified, using random initialization" << std::endl;
            std::cout << "[DQNPolicy] To use trained model: ./jetson_dqn <ip> -p dqn -m models/dqn_best.pt" << std::endl;
        }
    }

    bool loadModel(const std::string& model_path) {
        try {
            std::cout << "[DQNPolicy] Loading model from: " << model_path << std::endl;
            agent_->load(model_path);
            model_loaded_ = true;
            std::cout << "[DQNPolicy] ✓ Model loaded successfully" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[DQNPolicy] ✗ Failed to load model: " << e.what() << std::endl;
            std::cerr << "[DQNPolicy] Continuing with random initialization" << std::endl;
            model_loaded_ = false;
            return false;
        }
    }

    int selectAction() override {
        // NOTA: Por ahora usamos estado dummy (todos ceros)
        // En el futuro, esto se reemplazaría con lecturas reales de sensores del EV3
        // que llegarían desde el bridge (comunicación bidireccional)
        torch::Tensor state = torch::zeros({4}, torch::kFloat32);

        // Mover estado al device correcto
        state = state.to(device_);

        // Seleccionar acción usando el agente DQN (código probado)
        // false = no training mode (greedy, sin epsilon exploration)
        int64_t action;
        {
            torch::NoGradGuard no_grad;  // Deshabilitar gradientes para inferencia
            action = agent_->select_action(state, false);
        }

        return static_cast<int>(action);
    }

    std::string getName() const override {
        return model_loaded_ ? "DQN (trained)" : "DQN (random init)";
    }

    bool isModelLoaded() const {
        return model_loaded_;
    }

private:
    std::unique_ptr<dqn::DQNAgent> agent_;
    torch::Device device_;
    bool model_loaded_;
};

// ============================================================================
// MAIN
// ============================================================================

void print_usage(const char* program_name) {
    std::cout << "Uso: " << program_name << " <laptop_ip> [opciones]" << std::endl;
    std::cout << std::endl;
    std::cout << "Argumentos:" << std::endl;
    std::cout << "  laptop_ip        IP de la laptop con el bridge (ej: 192.168.1.100)" << std::endl;
    std::cout << std::endl;
    std::cout << "Opciones:" << std::endl;
    std::cout << "  -p <policy>      Política de selección (default: random)" << std::endl;
    std::cout << "                   random  = Acciones aleatorias (testing)" << std::endl;
    std::cout << "                   dqn     = DQN con red neuronal" << std::endl;
    std::cout << "  -m <model>       Ruta del modelo .pt (solo con -p dqn)" << std::endl;
    std::cout << std::endl;
    std::cout << "Ejemplos:" << std::endl;
    std::cout << "  " << program_name << " 192.168.1.100" << std::endl;
    std::cout << "  " << program_name << " 192.168.1.100 -p random" << std::endl;
    std::cout << "  " << program_name << " 192.168.1.100 -p dqn" << std::endl;
    std::cout << "  " << program_name << " 192.168.1.100 -p dqn -m models/dqn_best.pt" << std::endl;
}

int main(int argc, char* argv[]) {
    // ========================================================================
    // 1. Parsear argumentos
    // ========================================================================
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string laptop_ip = argv[1];
    std::string policy_name = "random";
    std::string model_path = "";

    // Parsear opciones
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            policy_name = argv[++i];
        } else if (arg == "-m" && i + 1 < argc) {
            model_path = argv[++i];
        }
    }

    // ========================================================================
    // 2. Banner
    // ========================================================================
    std::cout << "=========================================================================" << std::endl;
    std::cout << "  Jetson DQN Agent - Control de EV3 vía UDP" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Laptop Bridge:    " << laptop_ip << ":" << UDP_PORT << std::endl;
    std::cout << "Frecuencia:       " << ACTION_FREQUENCY << " Hz" << std::endl;
    std::cout << "Política:         " << policy_name << std::endl;
    std::cout << "Presiona Ctrl+C para detener" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << std::endl;

    // ========================================================================
    // 3. Configurar señal de interrupción
    // ========================================================================
    signal(SIGINT, signalHandler);

    // ========================================================================
    // 4. Crear política de selección
    // ========================================================================
    std::unique_ptr<Policy> policy;

    if (policy_name == "random") {
        policy = std::make_unique<RandomPolicy>();
        std::cout << "[Policy] Usando política aleatoria (testing mode)" << std::endl;
    } else if (policy_name == "dqn") {
        std::cout << "[Policy] Usando política DQN (código probado de jetson_test)" << std::endl;
        policy = std::make_unique<DQNPolicy>(model_path);
    } else {
        std::cerr << "[ERROR] Política desconocida: " << policy_name << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // ========================================================================
    // 5. Inicializar UDP sender
    // ========================================================================
    UDPSender udp(laptop_ip, UDP_PORT);
    if (!udp.isConnected()) {
        std::cerr << "[ERROR] No se pudo inicializar UDP sender" << std::endl;
        return 1;
    }

    // Enviar STOP inicial para asegurar que el robot está detenido
    std::cout << "[Init] Enviando STOP inicial..." << std::endl;
    udp.send(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ========================================================================
    // 6. Loop principal
    // ========================================================================
    std::cout << "\n[Running] Iniciando loop de control..." << std::endl;
    std::cout << "=========================================================================" << std::endl;

    auto delay = std::chrono::milliseconds(1000 / ACTION_FREQUENCY);
    int step = 0;

    while (running) {
        auto start_time = std::chrono::steady_clock::now();

        // Seleccionar acción usando la política
        int action = policy->selectAction();

        // Enviar acción por UDP
        if (!udp.send(action)) {
            std::cerr << "[WARN] Fallo al enviar acción " << action << std::endl;
        }

        step++;

        // Logging periódico de estadísticas
        if (step % 50 == 0) {
            std::cout << "\n[Stats] Steps ejecutados: " << step << std::endl;
            std::cout << "=========================================================================" << std::endl;
        }

        // Mantener frecuencia de acciones
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        if (elapsed < delay) {
            std::this_thread::sleep_for(delay - elapsed);
        }
    }

    // ========================================================================
    // 7. Cleanup
    // ========================================================================
    std::cout << "\n\n[Shutdown] Limpiando recursos..." << std::endl;
    std::cout << "  Total steps ejecutados: " << step << std::endl;

    // El destructor de UDPSender enviará STOP automáticamente

    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Programa terminado correctamente" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    return 0;
}
