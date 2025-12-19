#include "utils/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

namespace utils {

Logger::Logger(const std::string& log_file)
    : file_logging_(!log_file.empty()) {

    if (file_logging_) {
        file_.open(log_file, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[Logger] Warning: Could not open log file: " << log_file << std::endl;
            file_logging_ = false;
        } else {
            std::cout << "[Logger] Logging to file: " << log_file << std::endl;
        }
    }
}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(Level::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void Logger::log_episode(int episode, float reward, float epsilon, float loss) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Episode " << std::setw(4) << episode
        << " | Reward: " << std::setw(7) << reward
        << " | Epsilon: " << std::setprecision(3) << epsilon;

    if (loss >= 0.0f) {
        oss << " | Loss: " << std::setprecision(4) << loss;
    }

    info(oss.str());
}

void Logger::log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string timestamp = get_timestamp();
    std::string level_str = level_to_string(level);

    std::string log_message = "[" + timestamp + "] [" + level_str + "] " + message;

    // Console output
    if (level == Level::ERROR) {
        std::cerr << log_message << std::endl;
    } else {
        std::cout << log_message << std::endl;
    }

    // File output
    if (file_logging_ && file_.is_open()) {
        file_ << log_message << std::endl;
        file_.flush();
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::level_to_string(Level level) const {
    switch (level) {
        case Level::INFO:    return "INFO";
        case Level::WARNING: return "WARN";
        case Level::ERROR:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

} // namespace utils
