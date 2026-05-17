#include "key_value_store.h"
#include "setup.h" // for GetADBKingPath()

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {
    const char* kStoreFileName = "key_value_store.txt";

    std::string GetStoreFilePath() {
        std::string base = GetADBKingPath();
#ifdef _WIN32
        return base + "\\" + kStoreFileName;
#else
        return base + "/" + kStoreFileName;
#endif
    }

    // Very small, robust parser for a simple key=value text file.
    std::unordered_map<std::string, std::string> LoadAll() {
        std::unordered_map<std::string, std::string> data;
        try {
            const std::string path = GetStoreFilePath();
            std::ifstream in(path);
            if (!in.is_open()) {
                // Ensure directory exists; file will be created on first write
                std::filesystem::create_directories(GetADBKingPath());
                return data;
            }

            std::string line;
            while (std::getline(in, line)) {
                // Skip empty or comment lines
                if (line.empty() || line[0] == '#') continue;

                // Find first '='
                size_t eq = line.find('=');
                if (eq == std::string::npos) continue;

                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);

                // Trim whitespace on both ends
                auto trim = [](std::string& s) {
                    size_t start = s.find_first_not_of(" \t\r\n");
                    size_t end = s.find_last_not_of(" \t\r\n");
                    if (start == std::string::npos || end == std::string::npos) { s.clear(); return; }
                    s = s.substr(start, end - start + 1);
                };
                trim(key);
                trim(value);
                if (!key.empty()) {
                    data[key] = value;
                }
            }
        } catch (...) {
            // Swallow and return what we have
        }
        return data;
    }

    bool SaveAll(const std::unordered_map<std::string, std::string>& data) {
        try {
            std::filesystem::create_directories(GetADBKingPath());
            const std::string path = GetStoreFilePath();
            std::ofstream out(path, std::ios::trunc);
            if (!out.is_open()) return false;
            out << "# ADBKing key-value store" << '\n';
            for (const auto& [k, v] : data) {
                out << k << '=' << v << '\n';
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    // Safe converters
    std::string ToString(int v) { return std::to_string(v); }
    std::string ToString(float v) {
        std::ostringstream oss; oss.setf(std::ios::fixed); oss.precision(6); oss << v; return oss.str();
    }
    std::string ToString(bool v) { return v ? "1" : "0"; }

    int ToInt(const std::string& s, int def) {
        try { return std::stoi(s); } catch (...) { return def; }
    }
    float ToFloat(const std::string& s, float def) {
        try { return std::stof(s); } catch (...) { return def; }
    }
    bool ToBool(const std::string& s, bool def) {
        if (s == "1" || s == "true" || s == "TRUE" || s == "True") return true;
        if (s == "0" || s == "false" || s == "FALSE" || s == "False") return false;
        return def;
    }
}

namespace KeyValueStore {

bool SetString(const std::string& key, const std::string& value) {
    auto data = LoadAll();
    data[key] = value;
    return SaveAll(data);
}

std::string GetString(const std::string& key, const std::string& defaultValue) {
    auto data = LoadAll();
    auto it = data.find(key);
    if (it == data.end()) return defaultValue;
    return it->second;
}

bool DeleteKey(const std::string& key) {
    auto data = LoadAll();
    auto it = data.find(key);
    if (it == data.end()) return false;
    data.erase(it);
    return SaveAll(data);
}

bool SetFloat(const std::string& key, float value) {
    return SetString(key, ToString(value));
}

float GetFloat(const std::string& key, float defaultValue) {
    return ToFloat(GetString(key, ToString(defaultValue)), defaultValue);
}

bool SetInt(const std::string& key, int value) {
    return SetString(key, ToString(value));
}

int GetInt(const std::string& key, int defaultValue) {
    return ToInt(GetString(key, ToString(defaultValue)), defaultValue);
}

bool SetBool(const std::string& key, bool value) {
    return SetString(key, ToString(value));
}

bool GetBool(const std::string& key, bool defaultValue) {
    return ToBool(GetString(key, ToString(defaultValue)), defaultValue);
}

}


