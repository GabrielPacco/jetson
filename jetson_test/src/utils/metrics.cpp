#include "utils/metrics.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iostream>
#include <limits>

namespace utils {

MetricsTracker::MetricsTracker()
    : best_reward_(-std::numeric_limits<float>::infinity()) {
}

void MetricsTracker::record_episode(float reward) {
    episode_rewards_.push_back(reward);

    if (reward > best_reward_) {
        best_reward_ = reward;
    }
}

void MetricsTracker::record_loss(float loss) {
    episode_losses_.push_back(loss);
}

float MetricsTracker::get_mean_reward(int window) const {
    if (episode_rewards_.empty()) {
        return 0.0f;
    }

    int start = std::max(0, static_cast<int>(episode_rewards_.size()) - window);
    int count = episode_rewards_.size() - start;

    float sum = std::accumulate(episode_rewards_.begin() + start, episode_rewards_.end(), 0.0f);
    return sum / count;
}

float MetricsTracker::get_mean_loss(int window) const {
    if (episode_losses_.empty()) {
        return 0.0f;
    }

    int start = std::max(0, static_cast<int>(episode_losses_.size()) - window);
    int count = episode_losses_.size() - start;

    float sum = std::accumulate(episode_losses_.begin() + start, episode_losses_.end(), 0.0f);
    return sum / count;
}

float MetricsTracker::get_best_reward() const {
    return best_reward_;
}

bool MetricsTracker::is_best_reward(float reward) {
    return reward >= best_reward_;
}

int MetricsTracker::get_episode_count() const {
    return episode_rewards_.size();
}

void MetricsTracker::save_to_file(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[MetricsTracker] Error: Could not open file for writing: " << filepath << std::endl;
        return;
    }

    file << "episode,reward,loss\n";
    for (size_t i = 0; i < episode_rewards_.size(); ++i) {
        file << i + 1 << "," << episode_rewards_[i] << ",";
        if (i < episode_losses_.size()) {
            file << episode_losses_[i];
        }
        file << "\n";
    }

    file.close();
    std::cout << "[MetricsTracker] Metrics saved to: " << filepath << std::endl;
}

} // namespace utils
