#include "utils/config_parser.h"
#include <iostream>
#include <vector>
#include <sstream>

namespace utils {

ConfigParser::ConfigParser(const std::string& config_file) {
    try {
        root_ = YAML::LoadFile(config_file);
        std::cout << "[ConfigParser] Loaded configuration from: " << config_file << std::endl;
    } catch (const YAML::Exception& e) {
        std::cerr << "[ConfigParser] Error loading config file: " << e.what() << std::endl;
        root_ = YAML::Node();  // Empty node
    }
}

template<typename T>
T ConfigParser::get(const std::string& key, T default_value) const {
    try {
        YAML::Node node = get_node(key);
        if (node.IsDefined()) {
            return node.as<T>();
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "[ConfigParser] Error reading key '" << key << "': " << e.what() << std::endl;
    }
    return default_value;
}

// Explicit template instantiations
template int ConfigParser::get<int>(const std::string&, int) const;
template float ConfigParser::get<float>(const std::string&, float) const;
template double ConfigParser::get<double>(const std::string&, double) const;
template bool ConfigParser::get<bool>(const std::string&, bool) const;
template std::string ConfigParser::get<std::string>(const std::string&, std::string) const;

bool ConfigParser::has(const std::string& key) const {
    try {
        YAML::Node node = get_node(key);
        return node.IsDefined();
    } catch (...) {
        return false;
    }
}

YAML::Node ConfigParser::get_node(const std::string& key) const {
    // Split key by '.' to navigate nested structure
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;

    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    // Navigate to the node
    YAML::Node node = root_;
    for (const auto& p : parts) {
        if (node[p]) {
            node = node[p];
        } else {
            return YAML::Node();  // Return undefined node
        }
    }

    return node;
}

} // namespace utils
