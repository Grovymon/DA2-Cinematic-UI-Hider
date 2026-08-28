#include "key_chord.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string_view>
#include <vector>

namespace da2ui {
namespace {

struct NamedKey {
    std::string_view name;
    std::uint16_t virtualKey;
    std::string_view canonicalName;
};

constexpr std::array<NamedKey, 48> kNamedKeys{{
    {"BACK", 0x08, "Backspace"}, {"BACKSPACE", 0x08, "Backspace"},
    {"TAB", 0x09, "Tab"}, {"ENTER", 0x0D, "Enter"},
    {"RETURN", 0x0D, "Enter"}, {"PAUSE", 0x13, "Pause"},
    {"CAPSLOCK", 0x14, "CapsLock"}, {"ESC", 0x1B, "Escape"},
    {"ESCAPE", 0x1B, "Escape"}, {"SPACE", 0x20, "Space"},
    {"PAGEUP", 0x21, "PageUp"}, {"PRIOR", 0x21, "PageUp"},
    {"PGUP", 0x21, "PageUp"}, {"PAGEDOWN", 0x22, "PageDown"},
    {"NEXT", 0x22, "PageDown"}, {"PGDN", 0x22, "PageDown"},
    {"END", 0x23, "End"}, {"HOME", 0x24, "Home"},
    {"LEFT", 0x25, "Left"}, {"UP", 0x26, "Up"},
    {"RIGHT", 0x27, "Right"}, {"DOWN", 0x28, "Down"},
    {"PRINTSCREEN", 0x2C, "PrintScreen"}, {"PRTSC", 0x2C, "PrintScreen"},
    {"SYSRQ", 0x2C, "PrintScreen"}, {"SNAPSHOT", 0x2C, "PrintScreen"},
    {"INSERT", 0x2D, "Insert"}, {"INS", 0x2D, "Insert"},
    {"DELETE", 0x2E, "Delete"}, {"DEL", 0x2E, "Delete"},
    {"MULTIPLY", 0x6A, "Multiply"}, {"NUMPADMULT", 0x6A, "Multiply"},
    {"ADD", 0x6B, "Add"}, {"NUMPADADD", 0x6B, "Add"},
    {"SUBTRACT", 0x6D, "Subtract"}, {"NUMPADSUB", 0x6D, "Subtract"},
    {"DECIMAL", 0x6E, "Decimal"}, {"DIVIDE", 0x6F, "Divide"},
    {"NUMPADDIV", 0x6F, "Divide"}, {"NUMPADENTER", 0x0D, "Enter"},
    {"SCROLLLOCK", 0x91, "ScrollLock"}, {"SEMICOLON", 0xBA, "Semicolon"},
    {"EQUALS", 0xBB, "Equals"}, {"COMMA", 0xBC, "Comma"},
    {"MINUS", 0xBD, "Minus"}, {"PERIOD", 0xBE, "Period"},
    {"SLASH", 0xBF, "Slash"}, {"GRAVE", 0xC0, "Grave"},
}};

std::string Trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string Upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

bool TryParseSingleKey(const std::string& input, std::uint16_t& virtualKey, std::string& canonicalName) {
    std::string key = Upper(Trim(input));
    key.erase(std::remove(key.begin(), key.end(), ' '), key.end());

    if (key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z') {
        virtualKey = static_cast<std::uint16_t>(key[0]);
        canonicalName = key;
        return true;
    }
    if (key.size() == 1 && key[0] >= '0' && key[0] <= '9') {
        virtualKey = static_cast<std::uint16_t>(key[0]);
        canonicalName = key;
        return true;
    }

    if (key.size() >= 2 && key.size() <= 3 && key[0] == 'F') {
        try {
            const int number = std::stoi(key.substr(1));
            if (number >= 1 && number <= 24) {
                virtualKey = static_cast<std::uint16_t>(0x70 + number - 1);
                canonicalName = "F" + std::to_string(number);
                return true;
            }
        } catch (...) {
        }
    }

    if (key.size() == 7 && key.rfind("NUMPAD", 0) == 0 && key[6] >= '0' && key[6] <= '9') {
        virtualKey = static_cast<std::uint16_t>(0x60 + key[6] - '0');
        canonicalName = key;
        return true;
    }

    for (const auto& namedKey : kNamedKeys) {
        if (key == namedKey.name) {
            virtualKey = namedKey.virtualKey;
            canonicalName = namedKey.canonicalName;
            return true;
        }
    }
    return false;
}

}  // namespace

std::string KeyChord::ToString() const {
    std::vector<std::string> parts;
    if (control) parts.emplace_back("Ctrl");
    if (alt) parts.emplace_back("Alt");
    if (shift) parts.emplace_back("Shift");
    parts.push_back(keyName);

    std::ostringstream output;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) output << '+';
        output << parts[index];
    }
    return output.str();
}

bool KeyChord::EquivalentTo(const KeyChord& other) const noexcept {
    return virtualKey == other.virtualKey && control == other.control &&
           alt == other.alt && shift == other.shift;
}

bool KeyChord::HasModifiers() const noexcept {
    return control || alt || shift;
}

bool TryParseKeyChord(const std::string& input, KeyChord& chord, std::string& error) {
    chord = {};
    error.clear();
    const std::string normalized = Trim(input);
    if (normalized.empty()) {
        error = "Key binding is empty.";
        return false;
    }
    if (Upper(normalized) == "(UNASSIGNED)") {
        error = "Key binding is unassigned.";
        return false;
    }

    std::string keyPart;
    std::size_t start = 0;
    while (start <= normalized.size()) {
        const std::size_t end = normalized.find('+', start);
        const std::string part = Trim(normalized.substr(start, end == std::string::npos ? end : end - start));
        const std::string upperPart = Upper(part);
        if (upperPart == "CTRL" || upperPart == "CONTROL" || upperPart == "LCTRL" || upperPart == "RCTRL") {
            chord.control = true;
        } else if (upperPart == "ALT" || upperPart == "LALT" || upperPart == "RALT") {
            chord.alt = true;
        } else if (upperPart == "SHIFT" || upperPart == "LSHIFT" || upperPart == "RSHIFT") {
            chord.shift = true;
        } else if (keyPart.empty()) {
            keyPart = part;
        } else {
            error = "A binding may contain only one non-modifier key: " + input;
            return false;
        }

        if (end == std::string::npos) break;
        start = end + 1;
    }

    if (keyPart.empty()) {
        error = "Binding contains modifiers but no key: " + input;
        return false;
    }

    constexpr std::string_view prefix = "KEYBOARD::BUTTON_";
    const std::string upperKeyPart = Upper(keyPart);
    if (upperKeyPart.rfind(prefix, 0) == 0) {
        keyPart = keyPart.substr(prefix.size());
    }

    if (!TryParseSingleKey(keyPart, chord.virtualKey, chord.keyName)) {
        error = "Unsupported key name: " + keyPart;
        return false;
    }
    return true;
}

}  // namespace da2ui
