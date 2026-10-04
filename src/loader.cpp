// version.dll for SnowRunner's Bin folder: forwards every export to the real version.dll and loads the .asi files
// that sit next to the game's exe. The real DLL is version_chain.dll from the same folder when another mod used the
// name first, otherwise System32\version.dll; an export the chain file lacks comes from System32.
//
// A copy of this loader may itself be the chain file (a player updating the mod renames the old version.dll as the
// instructions for another mod's file say): under that name it forwards straight to System32 and loads no .asi files,
// so the chain ends there. Without that rule it would find itself as its own chain file and every call would jump to
// its own stub forever.
//
// The .asi files are loaded from a thread, so they load after the loader lock of this DLL's own load is released.
// Log: AsiLoader.log next to the exe, rewritten each start.
#include <windows.h>
#include <stdio.h>
#include <string>
#include "version_names.h"

extern "C" void *g_verReal[64] = {};

static std::wstring g_dir;      // the folder of this DLL, with the last backslash; empty = not known, nothing is loaded from it
static std::wstring g_forwards; // for the log: where the calls go

static void Log(const wchar_t *fmt, ...)
{
    static bool first = true;
    FILE *f = nullptr;
    if (_wfopen_s(&f, (g_dir + L"AsiLoader.log").c_str(), first ? L"w, ccs=UTF-8" : L"a, ccs=UTF-8") != 0 || !f) return;
    first = false;
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(f, fmt, ap);
    va_end(ap);
    fputwc(L'\n', f);
    fclose(f);
}

// A module's full path, however long; empty when Windows does not give it.
static std::wstring ModulePath(HMODULE m)
{
    std::wstring p(MAX_PATH, L'\0');
    for (;;)
    {
        const DWORD n = GetModuleFileNameW(m, &p[0], (DWORD)p.size());
        if (!n) return std::wstring();
        if (n < p.size()) { p.resize(n); return p; }
        if (p.size() >= 32768) return std::wstring();
        p.resize(p.size() * 2);
    }
}

// Fills g_verReal. chained: this DLL is itself somebody's chain file, so it goes to System32 only.
static bool LoadRealVersion(HINSTANCE self, bool chained)
{
    wchar_t sys[MAX_PATH] = {};
    const UINT n = GetSystemDirectoryW(sys, MAX_PATH);
    if (!n || n >= MAX_PATH) return false;
    const HMODULE system = LoadLibraryW((std::wstring(sys) + L"\\version.dll").c_str());
    if (!system) return false;
    HMODULE chain = nullptr;
    if (!chained && !g_dir.empty())
    {
        chain = LoadLibraryW((g_dir + L"version_chain.dll").c_str());
        if (chain == self || chain == system) chain = nullptr;
    }
    int fromChain = 0;
    for (int i = 0; i < kVerCount; i++)
    {
        void *p = chain ? (void *)GetProcAddress(chain, kVerNames[i]) : nullptr;
        if (p) fromChain++;
        else p = (void *)GetProcAddress(system, kVerNames[i]);
        if (!p) return false;
        g_verReal[i] = p;
    }
    wchar_t note[160];
    if (chain) swprintf_s(note, L"version_chain.dll next to the exe (%d of %d exports; the rest from System32)", fromChain, kVerCount);
    else swprintf_s(note, L"System32\\version.dll%s", chained ? L" (this file is a chain file itself)" : L"");
    g_forwards = note;
    return true;
}

static DWORD WINAPI LoadAsiFiles(void *)
{
    Log(L"asi loader in %s", ModulePath(nullptr).c_str());
    Log(L"version.dll calls go to %s", g_forwards.c_str());
    WIN32_FIND_DATAW fd = {};
    const HANDLE h = FindFirstFileW((g_dir + L"*.asi").c_str(), &fd);
    int loaded = 0, failed = 0;
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            // the pattern also finds names that only start their extension with .asi (x.asi_off, x.asi~: a mod switched
            // off by renaming), through their short names: only what ends in .asi is loaded
            const size_t len = wcslen(fd.cFileName);
            if (len < 5 || _wcsicmp(fd.cFileName + len - 4, L".asi")) { Log(L"left alone: %s (does not end in .asi)", fd.cFileName); continue; }
            const HMODULE m = LoadLibraryW((g_dir + fd.cFileName).c_str());
            if (m) { loaded++; Log(L"loaded %s at %p", fd.cFileName, (void *)m); }
            else { failed++; Log(L"FAILED %s, error %lu", fd.cFileName, GetLastError()); }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    Log(L"done: %d loaded, %d failed", loaded, failed);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(inst);
    // without this DLL's own full path nothing is loaded by a name alone (that would search the current directory
    // and the path): the calls then go to System32 and no .asi is loaded
    const std::wstring path = ModulePath(inst);
    const size_t slash = path.find_last_of(L"\\/");
    bool chained = false;
    if (slash != std::wstring::npos)
    {
        g_dir = path.substr(0, slash + 1);
        chained = !_wcsicmp(path.c_str() + slash + 1, L"version_chain.dll");
    }
    if (!LoadRealVersion(inst, chained)) return FALSE;
    if (g_dir.empty() || chained) return TRUE;
    const HANDLE t = CreateThread(nullptr, 0, LoadAsiFiles, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    return TRUE;
}
