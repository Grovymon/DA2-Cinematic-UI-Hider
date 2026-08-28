#include "da2_bindings.hpp"

#include <array>
#include <map>

namespace da2ui {
namespace {

std::string Trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

}  // namespace

bool TryReadDa2HideBinding(std::istream& stream, KeyChord& chord, std::string& detail) {
    std::map<std::string, std::string> candidates;
    std::string line;
    while (std::getline(stream, line)) {
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || equals == 0) continue;
        const std::string name = Trim(line.substr(0, equals));
        if (name == "HideMainInterface_0" || name == "HideMainInterface_1" || name == "HideMainInterface") {
            candidates[name] = Trim(line.substr(equals + 1));
        }
    }

    constexpr std::array<const char*, 3> order{{
        "HideMainInterface_0", "HideMainInterface_1", "HideMainInterface"
    }};
    for (const char* name : order) {
        const auto candidate = candidates.find(name);
        if (candidate == candidates.end()) continue;
        std::string parseError;
        if (TryParseKeyChord(candidate->second, chord, parseError)) {
            detail = std::string(name) + "=" + candidate->second;
            return true;
        }
    }

    detail = "No assigned, supported HideMainInterface binding was found.";
    return false;
}

}  // namespace da2ui
