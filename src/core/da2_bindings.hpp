#pragma once

#include "key_chord.hpp"

#include <istream>
#include <string>

namespace da2ui {

[[nodiscard]] bool TryReadDa2HideBinding(
    std::istream& stream,
    KeyChord& chord,
    std::string& detail);

}  // namespace da2ui
