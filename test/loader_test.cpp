// Offline test of out\version.dll. Built twice:
//   loader_test.exe   imports version.dll (the proxy next to it), asks for kernel32.dll's file version through it,
//                     then waits for the marker the test .asi writes.
//   probe.asi         (LOADER_TEST_ASI) writes asi_marker.txt next to itself when loaded: the id of the process it
//                     was loaded into, so a marker left by another run does not count.
// build.bat also puts the three files into out\test\chain with a second copy of the proxy as version_chain.dll: run
// there, the same test checks that a chain of two of these loaders ends in System32 (it once ran in a circle).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

static std::wstring DirOf(HMODULE m)
{
    wchar_t p[MAX_PATH] = {};
    GetModuleFileNameW(m, p, MAX_PATH);
    std::wstring d = p;
    d.resize(d.find_last_of(L"\\/") + 1);
    return d;
}

#ifdef LOADER_TEST_ASI
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        FILE *f = nullptr;
        if (_wfopen_s(&f, (DirOf(inst) + L"asi_marker.txt").c_str(), L"w") == 0 && f) { fprintf(f, "%lu", GetCurrentProcessId()); fclose(f); }
    }
    return TRUE;
}
#else
struct Version { bool wide, ansi; WORD major, minor, build; };

// The four exports the game itself imports, through the proxy.
static DWORD WINAPI AskVersion(void *out)
{
    Version &v = *(Version *)out;
    wchar_t sys[MAX_PATH] = {};
    GetSystemDirectoryW(sys, MAX_PATH);
    const std::wstring k32 = std::wstring(sys) + L"\\kernel32.dll";
    DWORD dummy = 0;
    const DWORD size = GetFileVersionInfoSizeW(k32.c_str(), &dummy);
    std::string buf(size, 0);
    VS_FIXEDFILEINFO *ffi = nullptr, *ffiA = nullptr;
    UINT len = 0;
    v.wide = size && GetFileVersionInfoW(k32.c_str(), 0, size, &buf[0]) && VerQueryValueW(&buf[0], L"\\", (void **)&ffi, &len) && ffi &&
             ffi->dwSignature == VS_FFI_SIGNATURE;
    v.ansi = v.wide && VerQueryValueA(&buf[0], "\\", (void **)&ffiA, &len) && ffiA == ffi;
    if (ffi) { v.major = HIWORD(ffi->dwFileVersionMS); v.minor = LOWORD(ffi->dwFileVersionMS); v.build = HIWORD(ffi->dwFileVersionLS); }
    return 0;
}

int main()
{
    int fails = 0;
    const std::wstring dir = DirOf(nullptr);

    wchar_t mod[MAX_PATH] = {};
    GetModuleFileNameW(GetModuleHandleW(L"version.dll"), mod, MAX_PATH);
    const bool ours = _wcsnicmp(mod, dir.c_str(), dir.size()) == 0;
    printf("%s version.dll in use: %ls\n", ours ? "ok  " : "FAIL", mod);
    fails += !ours;

    // on a thread with a time limit: a proxy that forwards to itself never comes back
    Version v = {};
    const HANDLE t = CreateThread(nullptr, 0, AskVersion, &v, 0, nullptr);
    const bool back = t && WaitForSingleObject(t, 10000) == WAIT_OBJECT_0;
    printf("%s forwarded calls%s: kernel32 version %u.%u.%u (wide %d, ansi %d)\n", back && v.wide && v.ansi ? "ok  " : "FAIL",
           back ? "" : " DID NOT RETURN in 10 s", v.major, v.minor, v.build, v.wide, v.ansi);
    if (!back) { printf("1 FAILED\n"); fflush(stdout); ExitProcess(1); }
    fails += !(v.wide && v.ansi);

    // the marker of this very process
    bool marker = false;
    for (int i = 0; i < 50 && !marker; i++)
    {
        FILE *f = nullptr;
        char text[32] = {};
        if (_wfopen_s(&f, (dir + L"asi_marker.txt").c_str(), L"r") == 0 && f)
        {
            fgets(text, sizeof text, f);
            fclose(f);
            marker = strtoul(text, nullptr, 10) == GetCurrentProcessId();
        }
        if (!marker) Sleep(100);
    }
    printf("%s probe.asi loaded into this process\n", marker ? "ok  " : "FAIL");
    fails += !marker;

    const bool mapped = GetModuleHandleW(L"probe.asi") != nullptr;
    printf("%s probe.asi mapped in the process\n", mapped ? "ok  " : "FAIL");
    fails += !mapped;

    const bool chain = GetModuleHandleW(L"version_chain.dll") != nullptr;
    printf("     chain file: %s\n", chain ? "version_chain.dll is loaded, the calls went through it" : "none");

    printf(fails ? "%d FAILED\n" : "ALL PASS\n", fails);
    return fails;
}
#endif
