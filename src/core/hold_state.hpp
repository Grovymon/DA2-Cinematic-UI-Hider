#pragma once

#include <cstdint>

namespace da2ui {

enum class HoldEvent {
    None,
    Down,
    ThresholdReached,
    Up,
};

class HoldState final {
public:
    explicit HoldState(std::uint64_t holdDurationMs) noexcept;

    HoldEvent Update(bool keyDown, std::uint64_t nowMs) noexcept;
    void Deactivate(bool keyDown) noexcept;

private:
    std::uint64_t holdDurationMs_;
    std::uint64_t downSinceMs_ = 0;
    bool wasDown_ = false;
    bool holdTriggered_ = false;
    bool requiresRelease_ = false;
};

class PressEdgeState final {
public:
    bool Update(bool keyDown) noexcept;
    void Deactivate(bool keyDown) noexcept;

private:
    bool wasDown_ = false;
    bool requiresRelease_ = false;
};

}  // namespace da2ui
