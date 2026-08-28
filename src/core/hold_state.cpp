#include "hold_state.hpp"

namespace da2ui {

HoldState::HoldState(const std::uint64_t holdDurationMs) noexcept
    : holdDurationMs_(holdDurationMs) {}

HoldEvent HoldState::Update(const bool keyDown, const std::uint64_t nowMs) noexcept {
    if (requiresRelease_) {
        if (!keyDown) {
            requiresRelease_ = false;
        }
        return HoldEvent::None;
    }

    if (!keyDown) {
        if (wasDown_) {
            wasDown_ = false;
            holdTriggered_ = false;
            downSinceMs_ = 0;
            return HoldEvent::Up;
        }
        return HoldEvent::None;
    }

    if (!wasDown_) {
        wasDown_ = true;
        holdTriggered_ = false;
        downSinceMs_ = nowMs;
        return HoldEvent::Down;
    }

    if (!holdTriggered_ && nowMs - downSinceMs_ >= holdDurationMs_) {
        holdTriggered_ = true;
        return HoldEvent::ThresholdReached;
    }

    return HoldEvent::None;
}

void HoldState::Deactivate(const bool keyDown) noexcept {
    wasDown_ = false;
    holdTriggered_ = false;
    downSinceMs_ = 0;
    if (keyDown) {
        requiresRelease_ = true;
    }
}

bool PressEdgeState::Update(const bool keyDown) noexcept {
    if (requiresRelease_) {
        if (!keyDown) {
            requiresRelease_ = false;
        }
        return false;
    }

    if (keyDown && !wasDown_) {
        wasDown_ = true;
        return true;
    }

    if (!keyDown) {
        wasDown_ = false;
    }
    return false;
}

void PressEdgeState::Deactivate(const bool keyDown) noexcept {
    wasDown_ = false;
    if (keyDown) {
        requiresRelease_ = true;
    }
}

}  // namespace da2ui
