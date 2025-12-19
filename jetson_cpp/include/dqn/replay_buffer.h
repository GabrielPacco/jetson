#ifndef DQN_REPLAY_BUFFER_H
#define DQN_REPLAY_BUFFER_H

#include <torch/torch.h>
#include <vector>
#include <deque>
#include <mutex>
#include <random>
#include "dqn/types.h"

namespace dqn {

/**
 * @brief Experience Replay Buffer for DQN
 *
 * Stores transitions (s, a, r, s', done) in a circular buffer with fixed capacity.
 * Provides random sampling for breaking temporal correlations during training.
 * Thread-safe for potential asynchronous data collection.
 */
class ReplayBuffer {
public:
    /**
     * @brief Construct a new Replay Buffer object
     *
     * @param capacity Maximum number of transitions to store
     */
    explicit ReplayBuffer(size_t capacity);

    /**
     * @brief Add a transition to the buffer
     *
     * If buffer is full, oldest transition is removed (FIFO).
     *
     * @param state Current state
     * @param action Action taken
     * @param reward Reward received
     * @param next_state Resulting next state
     * @param done Whether episode ended
     */
    void push(const torch::Tensor& state, int64_t action, float reward,
              const torch::Tensor& next_state, bool done);

    /**
     * @brief Sample a random batch of transitions
     *
     * @param batch_size Number of transitions to sample
     * @return TransitionBatch Batch of transitions as tensors
     * @throws std::runtime_error if buffer has fewer than batch_size transitions
     */
    TransitionBatch sample(size_t batch_size);

    /**
     * @brief Get current number of transitions in buffer
     *
     * @return size_t Number of stored transitions
     */
    size_t size() const;

    /**
     * @brief Check if buffer has enough transitions to sample
     *
     * @param batch_size Required batch size
     * @return true if buffer.size() >= batch_size
     */
    bool can_sample(size_t batch_size) const;

    /**
     * @brief Clear all transitions from buffer
     */
    void clear();

private:
    size_t capacity_;                           // Maximum buffer capacity
    std::deque<Transition> buffer_;             // Circular buffer using deque
    mutable std::mutex mutex_;                  // Thread safety
    std::mt19937 rng_;                          // Random number generator
};

} // namespace dqn

#endif // DQN_REPLAY_BUFFER_H
