/**
 * @file train_simulation.cpp
 * @brief Entrenamiento DQN en SIMULACIÓN (sin robot físico)
 *
 * Prueba el algoritmo DQN usando CartPole simulado.
 * NO requiere Bluetooth ni robot Lego.
 */

#include <iostream>
#include <torch/torch.h>
#include <memory>

#include "dqn/agent.h"
#include "environment/cartpole_env.h"
#include "utils/logger.h"
#include "utils/metrics.h"

int main(int argc, char* argv[]) {
    std::cout << "=========================================================================" << std::endl;
    std::cout << "  DQN Training - SIMULATION MODE (CartPole)" << std::endl;
    std::cout << "  Sin robot físico - Solo prueba de algoritmo" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    // Configuración
    int num_episodes = (argc > 1) ? std::atoi(argv[1]) : 500;
    int max_steps = 500;

    // Device
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::cout << "[Device] " << device << std::endl;

    // Crear entorno SIMULADO (sin Bluetooth)
    std::cout << "[Environment] Creando entorno CartPole simulado..." << std::endl;
    auto env = std::make_unique<environment::CartPoleEnv>(max_steps);

    // Crear agente DQN
    dqn::Hyperparameters params;
    params.learning_rate = 0.001f;
    params.gamma = 0.99f;
    params.epsilon_start = 1.0f;
    params.epsilon_end = 0.01f;
    params.epsilon_decay = 0.995f;
    params.batch_size = 64;
    params.buffer_capacity = 10000;
    params.target_update_freq = 10;
    params.hidden_dim1 = 128;
    params.hidden_dim2 = 128;

    std::cout << "[Agent] Creando DQN agent..." << std::endl;
    dqn::DQNAgent agent(env->state_dim(), env->action_dim(), params, device);

    // Logger y métricas
    utils::Logger logger("simulation_training.log");
    utils::MetricsTracker metrics;

    std::cout << "\n[Training] Iniciando entrenamiento simulado..." << std::endl;
    std::cout << "  Episodios: " << num_episodes << std::endl;
    std::cout << "  Objetivo: Recompensa promedio >= 195" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    // Training loop
    for (int episode = 1; episode <= num_episodes; ++episode) {
        torch::Tensor state = env->reset();
        float episode_reward = 0.0f;
        float episode_loss = 0.0f;
        int loss_count = 0;

        for (int step = 0; step < max_steps; ++step) {
            int64_t action = agent.select_action(state, true);
            auto result = env->step(action);

            agent.store_transition(state, action, result.reward, result.next_state, result.done);

            float loss = agent.train_step();
            if (loss >= 0.0f) {
                episode_loss += loss;
                loss_count++;
            }

            state = result.next_state;
            episode_reward += result.reward;

            if (result.done) break;
        }

        agent.decay_epsilon();

        if (episode % params.target_update_freq == 0) {
            agent.update_target_network();
        }

        float avg_loss = (loss_count > 0) ? (episode_loss / loss_count) : 0.0f;
        metrics.record_episode(episode_reward);
        if (avg_loss > 0.0f) metrics.record_loss(avg_loss);

        // Log cada 10 episodios
        if (episode % 10 == 0) {
            float mean_reward = metrics.get_mean_reward(100);
            logger.log_episode(episode, episode_reward, agent.get_epsilon(), avg_loss);
            std::cout << "  Reward medio (100 eps): " << mean_reward << std::endl;

            // Guardar mejor modelo
            if (metrics.is_best_reward(episode_reward)) {
                agent.save("models/dqn_simulation_best.pt");
            }
        }

        // Check si resolvió
        if (episode >= 100) {
            float mean_reward = metrics.get_mean_reward(100);
            if (mean_reward >= 195.0f) {
                std::cout << "\n🎉 RESUELTO en episodio " << episode << "!" << std::endl;
                std::cout << "   Recompensa promedio: " << mean_reward << std::endl;
                break;
            }
        }
    }

    // Guardar modelo final
    agent.save("models/dqn_simulation_final.pt");
    metrics.save_to_file("simulation_metrics.csv");

    std::cout << "\n=========================================================================" << std::endl;
    std::cout << "  Entrenamiento Simulado Completado" << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "Mejor recompensa: " << metrics.get_best_reward() << std::endl;
    std::cout << "Epsilon final: " << agent.get_epsilon() << std::endl;
    std::cout << "\nModelos guardados:" << std::endl;
    std::cout << "  - models/dqn_simulation_best.pt" << std::endl;
    std::cout << "  - models/dqn_simulation_final.pt" << std::endl;
    std::cout << "=========================================================================" << std::endl;

    return 0;
}
