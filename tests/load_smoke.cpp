#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int main() {
    const HMODULE mod = LoadLibraryW(L"DA2CinematicUIHider.asi");
    if (mod == nullptr) {
        return static_cast<int>(GetLastError());
    }

    // The ASI worker validates that its host is named DragonAge2.exe, reads the
    // adjacent INI, and writes its startup messages before this process exits.
    Sleep(750);
    return 0;
}
