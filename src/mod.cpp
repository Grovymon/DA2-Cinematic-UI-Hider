#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#include "core/da2_bindings.hpp"
#include "core/hold_state.hpp"
#include "core/key_chord.hpp"

#include <atomic>
#include <cwchar>
#include <cwctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr wchar_t kModName[] = L"DA2CinematicUIHider";
constexpr char kVersion[] = "0.1.0";

HMODULE g_module = nullptr;
std::atomic<bool> g_shutdown{false};

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
                                              nullptr, 0, nullptr, nullptr);
    if (required <= 0) return {};
    std::string output(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
                        output.data(), required, nullptr, nullptr);
    return output;
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int required = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    if (required <= 0) return {};
    std::wstring output(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), output.data(), required);
    return output;
}

std::filesystem::path ModuleDirectory() {
    std::vector<wchar_t> buffer(512);
    for (;;) {
        const DWORD length = GetModuleFileNameW(g_module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) return {};
        if (length < buffer.size() - 1) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::filesystem::path ExecutablePath() {
    std::vector<wchar_t> buffer(512);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) return {};
        if (length < buffer.size() - 1) return std::filesystem::path(std::wstring(buffer.data(), length));
        buffer.resize(buffer.size() * 2);
    }
}

class Logger final {
public:
    Logger(const bool enabled, std::filesystem::path path) : enabled_(enabled), path_(std::move(path)) {}

    void Write(const std::string& message) const {
        if (!enabled_) return;
        SYSTEMTIME now{};
        GetLocalTime(&now);
        char prefix[64]{};
        wsprintfA(prefix, "%04u-%02u-%02u %02u:%02u:%02u.%03u | ",
                  now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
        const std::string line = std::string(prefix) + message + "\r\n";
        const HANDLE file = CreateFileW(path_.c_str(), FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;
        DWORD written = 0;
        WriteFile(file, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
        CloseHandle(file);
    }

private:
    bool enabled_;
    std::filesystem::path path_;
};

struct Config final {
    da2ui::KeyChord toggleKey;
    da2ui::KeyChord fallbackHideKey;
    da2ui::KeyChord emergencyKey;
    std::uint64_t holdDurationMs = 300;
    DWORD pollIntervalMs = 10;
    bool autoDetectHideKey = true;
    bool onlyWhenFocused = true;
    bool debug = false;
    std::filesystem::path keyBindingsPath;
};

std::wstring ReadIniString(const std::filesystem::path& iniPath, const wchar_t* key, const wchar_t* fallback) {
    std::vector<wchar_t> buffer(1024);
    const DWORD length = GetPrivateProfileStringW(L"General", key, fallback, buffer.data(),
                                                   static_cast<DWORD>(buffer.size()), iniPath.c_str());
    return std::wstring(buffer.data(), length);
}

bool ReadIniBool(const std::filesystem::path& iniPath, const wchar_t* key, const bool fallback) {
    std::wstring value = ReadIniString(iniPath, key, fallback ? L"true" : L"false");
    for (wchar_t& character : value) character = static_cast<wchar_t>(towlower(character));
    return value == L"true" || value == L"1" || value == L"yes" || value == L"on";
}

std::filesystem::path DefaultKeyBindingsPath() {
    wchar_t documents[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documents))) return {};
    return std::filesystem::path(documents) / L"BioWare" / L"Dragon Age 2" / L"Settings" / L"KeyBindings.ini";
}

bool ParseConfiguredKey(const std::filesystem::path& iniPath, const wchar_t* setting,
                        const wchar_t* fallback, da2ui::KeyChord& output, std::string& error) {
    return da2ui::TryParseKeyChord(WideToUtf8(ReadIniString(iniPath, setting, fallback)), output, error);
}

bool LoadConfig(const std::filesystem::path& iniPath, Config& config, std::string& error) {
    if (!std::filesystem::exists(iniPath)) {
        error = "Configuration file not found: " + WideToUtf8(iniPath.wstring());
        return false;
    }
    if (!ParseConfiguredKey(iniPath, L"ToggleKey", L"F12", config.toggleKey, error)) {
        error = "ToggleKey: " + error;
        return false;
    }
    if (!ParseConfiguredKey(iniPath, L"HideInterfaceKey", L"V", config.fallbackHideKey, error)) {
        error = "HideInterfaceKey: " + error;
        return false;
    }
    if (!ParseConfiguredKey(iniPath, L"EmergencyRestoreKey", L"F11", config.emergencyKey, error)) {
        error = "EmergencyRestoreKey: " + error;
        return false;
    }

    const int holdDuration = GetPrivateProfileIntW(L"General", L"HoldDurationMs", 300, iniPath.c_str());
    if (holdDuration < 50 || holdDuration > 10000) {
        error = "HoldDurationMs must be from 50 to 10000.";
        return false;
    }
    config.holdDurationMs = static_cast<std::uint64_t>(holdDuration);

    const int pollInterval = GetPrivateProfileIntW(L"General", L"PollIntervalMs", 10, iniPath.c_str());
    if (pollInterval < 5 || pollInterval > 100) {
        error = "PollIntervalMs must be from 5 to 100.";
        return false;
    }
    config.pollIntervalMs = static_cast<DWORD>(pollInterval);
    config.autoDetectHideKey = ReadIniBool(iniPath, L"AutoDetectHideInterfaceKey", true);
    config.onlyWhenFocused = ReadIniBool(iniPath, L"OnlyWhenGameFocused", true);
    config.debug = ReadIniBool(iniPath, L"Debug", false);

    const std::wstring bindingsSetting = ReadIniString(iniPath, L"KeyBindingsPath", L"Auto");
    if (_wcsicmp(bindingsSetting.c_str(), L"Auto") == 0) {
        config.keyBindingsPath = DefaultKeyBindingsPath();
    } else {
        std::vector<wchar_t> expanded(32768);
        const DWORD length = ExpandEnvironmentStringsW(bindingsSetting.c_str(), expanded.data(),
                                                        static_cast<DWORD>(expanded.size()));
        config.keyBindingsPath = length > 0 && length < expanded.size()
            ? std::filesystem::path(expanded.data()) : std::filesystem::path(bindingsSetting);
    }
    return true;
}

bool IsKeyDown(const std::uint16_t virtualKey) {
    return (GetAsyncKeyState(static_cast<int>(virtualKey)) & 0x8000) != 0;
}

bool IsChordDown(const da2ui::KeyChord& chord) {
    if (!IsKeyDown(chord.virtualKey)) return false;
    if (chord.control && !IsKeyDown(VK_CONTROL)) return false;
    if (chord.alt && !IsKeyDown(VK_MENU)) return false;
    if (chord.shift && !IsKeyDown(VK_SHIFT)) return false;
    return true;
}

bool IsExtendedKey(const std::uint16_t virtualKey) {
    return virtualKey == VK_PRIOR || virtualKey == VK_NEXT || virtualKey == VK_END ||
           virtualKey == VK_HOME || virtualKey == VK_LEFT || virtualKey == VK_UP ||
           virtualKey == VK_RIGHT || virtualKey == VK_DOWN || virtualKey == VK_INSERT ||
           virtualKey == VK_DELETE || virtualKey == VK_DIVIDE;
}

INPUT MakeKeyInput(const std::uint16_t virtualKey, const bool keyUp) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    const UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
    if (scanCode != 0) {
        input.ki.wVk = 0;
        input.ki.wScan = static_cast<WORD>(scanCode);
        input.ki.dwFlags |= KEYEVENTF_SCANCODE;
        if (IsExtendedKey(virtualKey)) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    } else {
        input.ki.wVk = virtualKey;
    }
    return input;
}

bool SendChord(const da2ui::KeyChord& chord, std::string& error) {
    std::vector<INPUT> inputs;
    if (chord.control) inputs.push_back(MakeKeyInput(VK_CONTROL, false));
    if (chord.alt) inputs.push_back(MakeKeyInput(VK_MENU, false));
    if (chord.shift) inputs.push_back(MakeKeyInput(VK_SHIFT, false));
    inputs.push_back(MakeKeyInput(chord.virtualKey, false));
    inputs.push_back(MakeKeyInput(chord.virtualKey, true));
    if (chord.shift) inputs.push_back(MakeKeyInput(VK_SHIFT, true));
    if (chord.alt) inputs.push_back(MakeKeyInput(VK_MENU, true));
    if (chord.control) inputs.push_back(MakeKeyInput(VK_CONTROL, true));

    SetLastError(ERROR_SUCCESS);
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size()) {
        error = "SendInput sent " + std::to_string(sent) + " of " + std::to_string(inputs.size()) +
                " events; Win32 error " + std::to_string(GetLastError()) + ".";
        return false;
    }
    return true;
}

bool IsGameFocused() {
    const HWND foreground = GetForegroundWindow();
    if (foreground == nullptr) return false;
    DWORD processId = 0;
    GetWindowThreadProcessId(foreground, &processId);
    return processId == GetCurrentProcessId();
}

DWORD WINAPI ModThread(void*) {
    const std::filesystem::path moduleDirectory = ModuleDirectory();
    const std::filesystem::path iniPath = moduleDirectory / (std::wstring(kModName) + L".ini");
    Config config;
    std::string configError;
    if (!LoadConfig(iniPath, config, configError)) {
        const Logger errorLogger(true, moduleDirectory / (std::wstring(kModName) + L".log"));
        errorLogger.Write("ERROR: " + configError);
        OutputDebugStringA(("DA2 Cinematic UI Hider: " + configError + "\n").c_str());
        return 1;
    }

    const Logger logger(config.debug, moduleDirectory / (std::wstring(kModName) + L".log"));
    logger.Write(std::string("DA2 Cinematic UI Hider v") + kVersion + " loaded");

    const std::filesystem::path executable = ExecutablePath();
    if (_wcsicmp(executable.filename().c_str(), L"DragonAge2.exe") != 0) {
        logger.Write("ERROR: Unsupported host executable: " + WideToUtf8(executable.filename().wstring()));
        return 1;
    }
    logger.Write("Dragon Age II detected");

    da2ui::KeyChord hideKey = config.fallbackHideKey;
    if (config.autoDetectHideKey && !config.keyBindingsPath.empty()) {
        std::ifstream bindings(config.keyBindingsPath);
        if (bindings) {
            da2ui::KeyChord detected;
            std::string detail;
            if (da2ui::TryReadDa2HideBinding(bindings, detected, detail)) {
                hideKey = detected;
                logger.Write("Auto-detected DA2 binding: " + detail + " -> " + hideKey.ToString());
            } else {
                logger.Write("Binding auto-detection fallback: " + detail);
            }
        } else {
            logger.Write("Binding auto-detection fallback: KeyBindings.ini was not found at " +
                         WideToUtf8(config.keyBindingsPath.wstring()));
        }
    }

    if (config.toggleKey.EquivalentTo(hideKey)) {
        logger.Write("ERROR: ToggleKey and resolved HideInterfaceKey must be different");
        return 1;
    }
    if (config.toggleKey.EquivalentTo(config.emergencyKey)) {
        logger.Write("ERROR: ToggleKey and EmergencyRestoreKey must be different");
        return 1;
    }
    if (config.toggleKey.HasModifiers()) {
        logger.Write("WARNING: A modifier-based ToggleKey may alter the DA2 binding sent while held");
    }

    logger.Write("Toggle key: " + config.toggleKey.ToString() +
                 "; hold duration: " + std::to_string(config.holdDurationMs) + " ms");
    logger.Write("DA2 Hide main interface key: " + hideKey.ToString());

    da2ui::HoldState holdState(config.holdDurationMs);
    da2ui::PressEdgeState emergencyState;
    bool trackedHidden = false;
    bool wasActive = false;

    while (!g_shutdown.load(std::memory_order_relaxed)) {
        const bool toggleDown = IsChordDown(config.toggleKey);
        const bool emergencyDown = IsChordDown(config.emergencyKey);
        const bool active = !config.onlyWhenFocused || IsGameFocused();

        if (!active) {
            holdState.Deactivate(toggleDown);
            emergencyState.Deactivate(emergencyDown);
            if (wasActive) logger.Write("Game focus lost; physical hold reset; tracked UI state unchanged");
            wasActive = false;
            Sleep(config.pollIntervalMs);
            continue;
        }

        if (!wasActive) logger.Write("Game focused");
        wasActive = true;

        const da2ui::HoldEvent event = holdState.Update(toggleDown, GetTickCount64());
        if (event == da2ui::HoldEvent::Down) {
            logger.Write(config.toggleKey.ToString() + " DOWN");
            logger.Write("Hold timer started");
        } else if (event == da2ui::HoldEvent::ThresholdReached) {
            logger.Write(config.toggleKey.ToString() + " hold threshold reached");
            std::string error;
            if (SendChord(hideKey, error)) {
                trackedHidden = !trackedHidden;
                logger.Write(std::string("GUI toggled ") + (trackedHidden ? "OFF" : "ON") + " (tracked state)");
            } else {
                logger.Write("ERROR: GUI toggle input failed: " + error);
            }
        } else if (event == da2ui::HoldEvent::Up) {
            logger.Write(config.toggleKey.ToString() + " UP");
            logger.Write("Hold state reset; tracked UI state unchanged");
        }

        if (emergencyState.Update(emergencyDown)) {
            logger.Write(config.emergencyKey.ToString() + " emergency restore requested");
            if (!trackedHidden) {
                logger.Write("Emergency restore did not send a blind toggle because tracked state is already visible");
            } else {
                std::string error;
                if (SendChord(hideKey, error)) {
                    trackedHidden = false;
                    logger.Write("GUI restore command sent; tracked state set to ON");
                } else {
                    logger.Write("ERROR: Emergency restore input failed: " + error);
                }
            }
        }

        Sleep(config.pollIntervalMs);
    }

    logger.Write("Mod worker stopped");
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        const HANDLE thread = CreateThread(nullptr, 0, ModThread, nullptr, 0, nullptr);
        if (thread != nullptr) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        g_shutdown.store(true, std::memory_order_relaxed);
    }
    return TRUE;
}
