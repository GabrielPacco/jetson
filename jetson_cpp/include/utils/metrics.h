#ifndef UTILS_METRICS_H
#define UTILS_METRICS_H

#include <vector>
#include <string>

namespace utils {

/**
 * @brief Training metrics tracker
 *
 * Tracks and computes statistics for training progress.
 */
class MetricsTracker {
public:
    MetricsTracker();

    /**
     * @brief Record an episode's reward
     *
     * @param reward Total episode reward
     */
    void record_episode(float reward);

    /**
     * @brief Record a training loss value
     *
     * @param loss Loss value
     */
    void record_loss(float loss);

    /**
     * @brief Get mean reward over last N episodes
     *
     * @param window Window size (default: 100)
     * @return float Mean reward
     */
    float get_mean_reward(int window = 100) const;

    /**
     * @brief Get mean loss over last N episodes
     *
     * @param window Window size (default: 100)
     * @return float Mean loss
     */
    float get_mean_loss(int window = 100) const;

    /**
     * @brief Get best reward achieved so far
     *
     * @return float Best reward
     */
    float get_best_reward() const;

    /**
     * @brief Check if current reward is the best
     *
     * @param reward Reward to check
     * @return true if this is the best reward
     */
    bool is_best_reward(float reward);

    /**
     * @brief Get total number of episodes recorded
     *
     * @return int Episode count
     */
    int get_episode_count() const;

    /**
     * @brief Save metrics to file
     *
     * @param filepath Path to save file
     */
    void save_to_file(const std::string& filepath) const;

private:
    std::vector<float> episode_rewards_;
    std::vector<float> episode_losses_;
    float best_reward_;
};

} // namespace utils

#endif // UTILS_METRICS_H
