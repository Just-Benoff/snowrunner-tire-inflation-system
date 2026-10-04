// ptrscan: read-only map of how the driven truck reaches its wheel objects in the running SnowRunner.exe.
// Known (exe stamp 0x6a607c05): the truck control global at RVA 0x2a8eb78, the current vehicle at control+8; wheel
// objects are 0x1D8 bytes with the vtable at RVA 0x2258918; the wheel setup's caller stores the wheel at model+0x2C8.
// Prints the current vehicle, every wheel object, where each wheel pointer is stored, and where pointers to those
// holders (as model = holder - 0x2C8) are stored, marking anything inside the vehicle object.
// build: cl /nologo /O2 /EHsc /MT tools\ptrscan.cpp /Fe:out\ptrscan.exe
// usage: out\ptrscan.exe [radius]      (only wheels of that radius, e.g. 0.47)
//        out\ptrscan.exe chain         (only the vehicle and its wheels)
//        out\ptrscan.exe names         (text reachable from the vehicle within two pointer steps: where the truck's
//                                       name is kept, for a per-truck setting)
#include <windows.h>
#include <tlhelp32.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

struct Region { uint64_t base; std::vector<BYTE> data; };

int main(int argc, char **argv)
{
    const float wantRadius = argc > 1 ? (float)atof(argv[1]) : -1.0f;
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe = { sizeof pe };
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) if (!_wcsicmp(pe.szExeFile, L"SnowRunner.exe")) pid = pe.th32ProcessID;
    CloseHandle(snap);
    if (!pid) { printf("no SnowRunner.exe\n"); return 2; }
    uint64_t base = 0;
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    MODULEENTRY32W me = { sizeof me };
    if (Module32FirstW(snap, &me)) base = (uint64_t)me.modBaseAddr;
    CloseHandle(snap);
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!proc || !base) { printf("OpenProcess or module lookup failed (%lu)\n", GetLastError()); return 2; }

    uint64_t control = 0, vehicle = 0;
    ReadProcessMemory(proc, (void *)(base + 0x2a8eb78), &control, 8, nullptr);
    if (control) ReadProcessMemory(proc, (void *)(control + 8), &vehicle, 8, nullptr);
    printf("base %llx  control %llx  vehicle %llx\n", base, control, vehicle);
    // the chain TirePressure.asi follows: vehicle+0x200 / +0x208 = begin / end of the wheel model array, each +0x2C8 -> wheel
    {
        uint64_t arr = 0, end = 0;
        if (vehicle) ReadProcessMemory(proc, (void *)(vehicle + 0x200), &arr, 8, nullptr);
        if (vehicle) ReadProcessMemory(proc, (void *)(vehicle + 0x208), &end, 8, nullptr);
        printf("chain: wheel models %llx .. %llx  wheels:", arr, end);
        for (uint64_t a = arr; arr && a < end && a < arr + 64 * 8; a += 8)
        {
            uint64_t wm = 0, w = 0, vt = 0;
            if (!ReadProcessMemory(proc, (void *)a, &wm, 8, nullptr) || !ReadProcessMemory(proc, (void *)(wm + 0x2C8), &w, 8, nullptr) ||
                !ReadProcessMemory(proc, (void *)w, &vt, 8, nullptr) || vt != base + 0x2258918)
                continue;
            printf(" %llx", w);
        }
        printf("\n");
    }
    if (argc > 1 && !strcmp(argv[1], "chain")) return 0; // only the chain: no copy of the game's memory
    if (argc > 1 && !strcmp(argv[1], "names"))
    {
        // Text a field can hold: a char or wchar_t string at a pointer, an MSVC std::string (up to 15 characters in
        // place, longer ones behind the pointer at +0 with the length at +0x10), or characters in the object itself.
        auto readAt = [&](uint64_t a, void *out, size_t n) { SIZE_T got = 0; return a >= 0x10000 && ReadProcessMemory(proc, (void *)a, out, n, &got) && got == n; };
        auto text = [](const BYTE *p, size_t max, int step, char *out) -> int {
            int n = 0;
            for (size_t i = 0; i + step <= max && n < 95; i += step)
            {
                const unsigned c = step == 1 ? p[i] : (unsigned)(p[i] | (p[i + 1] << 8));
                if (c == 0) break;
                if (c < 0x20 || c > 0x7E) return 0;
                out[n++] = (char)c;
            }
            out[n] = 0;
            return n >= 4 ? n : 0;
        };
        auto show = [&](const char *path, uint64_t field) {
            BYTE buf[200] = {};
            char s[96];
            // the field holds a pointer to text
            uint64_t p = 0;
            if (readAt(field, &p, 8) && readAt(p, buf, sizeof buf))
            {
                if (text(buf, sizeof buf, 1, s)) printf("%s -> \"%s\"\n", path, s);
                else if (text(buf, sizeof buf, 2, s)) printf("%s -> L\"%s\"\n", path, s);
            }
            // the field is a std::string or holds characters itself
            BYTE in[32] = {};
            uint64_t len = 0;
            if (readAt(field, in, sizeof in) && (len = *(const uint64_t *)(in + 0x10)) >= 4 && len <= 15 && *(const uint64_t *)(in + 0x18) == 15 && text(in, (size_t)len, 1, s))
                printf("%s = std::string \"%s\"\n", path, s);
        };
        char path[128];
        for (uint64_t a = 0; vehicle && a < 0x2000; a += 8)
        {
            sprintf_s(path, "vehicle+0x%llX", a);
            show(path, vehicle + a);
            uint64_t p = 0;
            if (!readAt(vehicle + a, &p, 8) || p < 0x10000 || (p >= vehicle && p < vehicle + 0x2000)) continue;
            for (uint64_t b = 0; b < 0x400; b += 8)
            {
                sprintf_s(path, "vehicle+0x%llX -> +0x%llX", a, b);
                show(path, p + b);
            }
        }
        return 0;
    }

    std::vector<Region> regions;
    MEMORY_BASIC_INFORMATION mbi;
    for (BYTE *a = nullptr; VirtualQueryEx(proc, a, &mbi, sizeof mbi) == sizeof mbi; a = (BYTE *)mbi.BaseAddress + mbi.RegionSize)
    {
        if (mbi.State != MEM_COMMIT || mbi.Type != MEM_PRIVATE || (mbi.Protect & 0xFF) != PAGE_READWRITE || (mbi.Protect & PAGE_GUARD)) continue;
        Region r{ (uint64_t)mbi.BaseAddress, std::vector<BYTE>(mbi.RegionSize) };
        SIZE_T got = 0;
        if (!ReadProcessMemory(proc, mbi.BaseAddress, r.data.data(), mbi.RegionSize, &got) || got < 0x200) continue;
        r.data.resize(got & ~(SIZE_T)7);
        regions.push_back(std::move(r));
    }

    auto holders = [&](uint64_t target, std::vector<uint64_t> &out) {
        for (const Region &r : regions)
            for (size_t i = 0; i + 8 <= r.data.size(); i += 8)
                if (*(const uint64_t *)(r.data.data() + i) == target) out.push_back(r.base + i);
    };
    auto inVehicle = [&](uint64_t a) { return vehicle && a >= vehicle && a < vehicle + 0x4000; };

    std::vector<uint64_t> wheels;
    for (const Region &r : regions)
        for (size_t i = 0; i + 0x1D8 <= r.data.size(); i += 8)
        {
            const BYTE *w = r.data.data() + i;
            float radius;
            memcpy(&radius, w + 0x94, 4);
            if (*(const uint64_t *)w != base + 0x2258918 || (wantRadius >= 0 && fabsf(radius - wantRadius) > 0.002f)) continue;
            wheels.push_back(r.base + i);
            printf("wheel %llx  radius %.4f  +10 %llx  +18 %llx  +20 %llx  +F8 %llx  +100 %llx  +108 %llx  +120 %llx\n", r.base + i, radius,
                   *(const uint64_t *)(w + 0x10), *(const uint64_t *)(w + 0x18), *(const uint64_t *)(w + 0x20), *(const uint64_t *)(w + 0xF8),
                   *(const uint64_t *)(w + 0x100), *(const uint64_t *)(w + 0x108), *(const uint64_t *)(w + 0x120));
        }
    printf("%zu wheel objects\n", wheels.size());

    for (uint64_t w : wheels)
    {
        std::vector<uint64_t> h1;
        holders(w, h1);
        printf("wheel %llx held at:", w);
        for (uint64_t h : h1) printf(" %llx%s", h, inVehicle(h) ? "(vehicle+)" : "");
        printf("\n");
        for (uint64_t h : h1)
        {
            std::vector<uint64_t> h2;
            holders(h - 0x2C8, h2);
            if (h2.empty()) continue;
            printf("    model %llx held at:", h - 0x2C8);
            for (uint64_t g : h2)
            {
                if (inVehicle(g)) printf(" vehicle+%llX", g - vehicle);
                else printf(" %llx", g);
            }
            printf("\n");
        }
    }
    CloseHandle(proc);
    return 0;
}
