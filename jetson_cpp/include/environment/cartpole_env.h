#ifndef ENVIRONMENT_CARTPOLE_ENV_H
#define ENVIRONMENT_CARTPOLE_ENV_H

#include "environment/environment_interface.h"
#include <random>

namespace environment {

/**
 * @brief CartPole Environment - Entorno Simulado
 *
 * Simulación del problema clásico CartPole para probar DQN sin robot físico.
 * NO requiere Bluetooth ni hardware.
 *
 * Estado (4D): [posición_carrito, velocidad_carrito, ángulo_poste, velocidad_angular]
 * Acciones (2): {0: izquierda, 1: derecha}
 */
class CartPoleEnv : public EnvironmentInterface {
public:
    explicit CartPoleEnv(int max_steps = 500);
    ~CartPoleEnv() override = default;

    torch::Tensor reset() override;
    StepResult step(int64_t action) override;
    int64_t state_dim() const override { return 4; }
    int64_t action_dim() const override { return 2; }
    void close() override {}

private:
    // Parámetros físicos
    static constexpr float GRAVITY = 9.8f;
    static constexpr float CART_MASS = 1.0f;
    static constexpr float POLE_MASS = 0.1f;
    static constexpr float POLE_LENGTH = 0.5f;
    static constexpr float FORCE_MAG = 10.0f;
    static constexpr float TAU = 0.02f;
    static constexpr float X_THRESHOLD = 2.4f;
    static constexpr float THETA_THRESHOLD = 0.2095f;  // ~12 grados

    float x_, x_dot_, theta_, theta_dot_;
    int step_count_, max_steps_;
    std::mt19937 rng_;

    void update_physics(float force);
    bool is_terminal() const;
    torch::Tensor get_state() const;
};

} // namespace environment

#endif
