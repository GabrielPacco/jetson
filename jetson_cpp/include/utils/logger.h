#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace utils {

/**
 * @brief Simple logger for DQN training
 *
 * Logs messages to both console and file with timestamps.
 */
class Logger {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR
    };

    /**
     * @brief Construct a new Logger
     *
     * @param log_file Path to log file (empty for console-only logging)
     */
    explicit Logger(const std::string& log_file = "");

    ~Logger();

    /**
     * @brief Log an info message
     *
     * @param message Message to log
     */
    void info(const std::string& message);

    /**
     * @brief Log a warning message
     *
     * @param message Message to log
     */
    void warning(const std::string& message);

    /**
     * @brief Log an error message
     *
     * @param message Message to log
     */
    void error(const std::string& message);

    /**
     * @brief Log training episode information
     *
     * @param episode Episode number
     * @param reward Total episode reward
     * @param epsilon Current epsilon value
     * @param loss Average loss (optional, -1 if not available)
     */
    void log_episode(int episode, float reward, float epsilon, float loss = -1.0f);

private:
    void log(Level level, const std::string& message);
    std::string get_timestamp() const;
    std::string level_to_string(Level level) const;

    std::ofstream file_;
    std::mutex mutex_;
    bool file_logging_;
};

} // namespace utils

#endif // UTILS_LOGGER_H
