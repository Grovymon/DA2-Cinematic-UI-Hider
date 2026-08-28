#pragma once

#include <cstdint>
#include <string>

namespace da2ui {

struct KeyChord final {
    std::uint16_t virtualKey = 0;
    std::string keyName;
    bool control = false;
    bool alt = false;
    bool shift = false;

    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] bool EquivalentTo(const KeyChord& other) const noexcept;
    [[nodiscard]] bool HasModifiers() const noexcept;
};

[[nodiscard]] bool TryParseKeyChord(
    const std::string& input,
    KeyChord& chord,
    std::string& error);

}  // namespace da2ui
