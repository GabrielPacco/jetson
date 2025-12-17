#ifndef UTILS_CONFIG_PARSER_H
#define UTILS_CONFIG_PARSER_H

#include <string>
#include <map>
#include <yaml-cpp/yaml.h>

namespace utils {

/**
 * @brief Simple YAML configuration parser
 *
 * Loads configuration from YAML file and provides type-safe getters.
 */
class ConfigParser {
public:
    /**
     * @brief Construct a new Config Parser
     *
     * @param config_file Path to YAML configuration file
     */
    explicit ConfigParser(const std::string& config_file);

    /**
     * @brief Get a configuration value
     *
     * @tparam T Value type
     * @param key Dot-separated key (e.g., "training.learning_rate")
     * @param default_value Default value if key not found
     * @return T Configuration value
     */
    template<typename T>
    T get(const std::string& key, T default_value = T()) const;

    /**
     * @brief Check if a key exists
     *
     * @param key Dot-separated key
     * @return true if key exists
     */
    bool has(const std::string& key) const;

private:
    YAML::Node get_node(const std::string& key) const;

    YAML::Node root_;
};

} // namespace utils

#endif // UTILS_CONFIG_PARSER_H
