#include "core/da2_bindings.hpp"
#include "core/hold_state.hpp"
#include "core/key_chord.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int g_tests = 0;

template <typename Expected, typename Actual>
void Equal(const Expected& expected, const Actual& actual) {
    if (!(expected == actual)) throw std::runtime_error("assertion failed");
}

void Run(const char* name, const std::function<void()>& test) {
    test();
    ++g_tests;
    std::cout << "PASS: " << name << '\n';
}

void FastTapDoesNotToggle() {
    da2ui::HoldState state(300);
    Equal(da2ui::HoldEvent::Down, state.Update(true, 1000));
    Equal(da2ui::HoldEvent::None, state.Update(true, 1050));
    Equal(da2ui::HoldEvent::Up, state.Update(false, 1051));
}

void ThresholdTogglesOnce() {
    da2ui::HoldState state(300);
    Equal(da2ui::HoldEvent::Down, state.Update(true, 1000));
    Equal(da2ui::HoldEvent::None, state.Update(true, 1299));
    Equal(da2ui::HoldEvent::ThresholdReached, state.Update(true, 1300));
}

void LongHoldTogglesOnce() {
    da2ui::HoldState state(300);
    int triggers = 0;
    state.Update(true, 0);
    for (std::uint64_t time = 10; time <= 5000; time += 10) {
        if (state.Update(true, time) == da2ui::HoldEvent::ThresholdReached) ++triggers;
    }
    Equal(1, triggers);
}

void ReleasePreservesStateAndSecondHoldToggles() {
    da2ui::HoldState state(300);
    bool trackedHidden = false;
    state.Update(true, 0);
    if (state.Update(true, 300) == da2ui::HoldEvent::ThresholdReached) trackedHidden = !trackedHidden;
    Equal(da2ui::HoldEvent::Up, state.Update(false, 301));
    Equal(true, trackedHidden);
    state.Update(true, 1000);
    if (state.Update(true, 1300) == da2ui::HoldEvent::ThresholdReached) trackedHidden = !trackedHidden;
    Equal(false, trackedHidden);
}

void FocusLossRequiresRelease() {
    da2ui::HoldState state(300);
    state.Update(true, 0);
    state.Deactivate(true);
    Equal(da2ui::HoldEvent::None, state.Update(true, 1000));
    Equal(da2ui::HoldEvent::None, state.Update(true, 2000));
    Equal(da2ui::HoldEvent::None, state.Update(false, 2001));
    Equal(da2ui::HoldEvent::Down, state.Update(true, 3000));
    Equal(da2ui::HoldEvent::ThresholdReached, state.Update(true, 3300));
}

void EmergencyIsEdgeTriggered() {
    da2ui::PressEdgeState state;
    Equal(true, state.Update(true));
    Equal(false, state.Update(true));
    Equal(false, state.Update(false));
    Equal(true, state.Update(true));
    state.Deactivate(true);
    Equal(false, state.Update(true));
    Equal(false, state.Update(false));
    Equal(true, state.Update(true));
}

void KeySyntaxParses() {
    da2ui::KeyChord chord;
    std::string error;
    Equal(true, da2ui::TryParseKeyChord("Alt + Keyboard::Button_Z", chord, error));
    Equal(std::string("Alt+Z"), chord.ToString());
    Equal(true, da2ui::TryParseKeyChord("Keyboard::Button_F12", chord, error));
    Equal(std::string("F12"), chord.ToString());
}

void PrimaryBindingWins() {
    std::istringstream input(
        "HideMainInterface_1=Alt + Keyboard::Button_Z\n"
        "HideMainInterface_0=Keyboard::Button_V\n");
    da2ui::KeyChord chord;
    std::string detail;
    Equal(true, da2ui::TryReadDa2HideBinding(input, chord, detail));
    Equal(std::string("V"), chord.ToString());
}

void SecondaryBindingFallback() {
    std::istringstream input(
        "HideMainInterface_0=(UNASSIGNED)\n"
        "HideMainInterface_1=Keyboard::Button_F10\n");
    da2ui::KeyChord chord;
    std::string detail;
    Equal(true, da2ui::TryReadDa2HideBinding(input, chord, detail));
    Equal(std::string("F10"), chord.ToString());
}

}  // namespace

int main() {
    try {
        Run("fast tap does not toggle", FastTapDoesNotToggle);
        Run("threshold toggles once", ThresholdTogglesOnce);
        Run("five-second hold toggles once", LongHoldTogglesOnce);
        Run("release preserves state and second hold toggles", ReleasePreservesStateAndSecondHoldToggles);
        Run("focus loss requires physical release", FocusLossRequiresRelease);
        Run("emergency press is edge-triggered", EmergencyIsEdgeTriggered);
        Run("DA2 key syntax parses", KeySyntaxParses);
        Run("DA2 primary binding wins", PrimaryBindingWins);
        Run("unassigned primary falls back to secondary", SecondaryBindingFallback);
        std::cout << "PASS: " << g_tests << " deterministic tests\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
