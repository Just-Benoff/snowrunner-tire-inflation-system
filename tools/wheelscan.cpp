// wheelscan: finds the game's wheel objects in the running SnowRunner.exe and prints their tire values; with --set it
// also writes a value into the selected wheels (the live experiment: does the game use a changed value?).
// Layout from the wheel setup code at RVA 0xc27000 (exe stamp 0x6a607c05), confirmed on a loaded map:
//   +0x78, +0x7C  SoftForceScale (two copies)      +0x90  width, +0x94  radius (less +0xB4 = the collision radius)
//   +0xA0  SubstanceFriction                       +0xA4  BodyFrictionAsphalt (times the winter multiplier)
//   +0xA8  10.0 and +0xAC 0.05, constants the setup writes: the signature searched for
//   +0xB0  Mass                                    +0xB4  RadiusOffset times the wheel scale
//   +0xD0  a flag byte                             +0xD1  IsIgnoreIce
// The objects exist only once a map is loaded.
// build: cl /nologo /O2 /EHsc /MT tools\wheelscan.cpp /Fe:out\wheelscan.exe
// usage: out\wheelscan.exe [--radius R] [--soft S] [--set field=value | field=xfactor]...
//        --radius / --soft   only wheels with that radius / softness (within 0.002)
//        fields: soft (both copies), sub, asphalt, offset, radius, width, mass
//        e.g. out\wheelscan.exe --radius 0.47 --set sub=x0.05      mud friction of those wheels to 5%
#include <windows.h>
#include <tlhelp32.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

static float F(const BYTE *p, size_t off) { float f; memcpy(&f, p + off, 4); return f; }

struct Field { const char *name; size_t off, off2; int where; }; // where: 0 the wheel object, 1 its Havok body, 2 the body's shape
static const Field kFields[] = { { "soft", 0x78, 0x7C, 0 }, { "sub", 0xA0, 0, 0 }, { "asphalt", 0xA4, 0, 0 }, { "offset", 0xB4, 0, 0 },
                                 { "radius", 0x94, 0, 0 }, { "width", 0x90, 0, 0 }, { "mass", 0xB0, 0, 0 },
                                 { "body", 0xCC, 0, 1 }, { "cyl", 0x28, 0, 2 } };
struct Change { const Field *f; bool scale; float v; };

int main(int argc, char **argv)
{
    float wantRadius = -1, wantSoft = -1;
    bool probe = false; // --probe: also print each wheel's Havok body and shape
    std::vector<Change> changes;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--radius") && i + 1 < argc) wantRadius = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--soft") && i + 1 < argc) wantSoft = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--probe")) probe = true;
        else if (!strcmp(argv[i], "--set") && i + 1 < argc)
        {
            const char *a = argv[++i], *eq = strchr(a, '=');
            const Field *f = nullptr;
            for (const Field &k : kFields) if (eq && strlen(k.name) == (size_t)(eq - a) && !strncmp(k.name, a, eq - a)) f = &k;
            if (!f) { printf("unknown field in %s\n", a); return 2; }
            const bool scale = eq[1] == 'x';
            changes.push_back({ f, scale, (float)atof(eq + (scale ? 2 : 1)) });
        }
        else { printf("unknown option %s\n", argv[i]); return 2; }
    }

    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe = { sizeof pe };
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) if (!_wcsicmp(pe.szExeFile, L"SnowRunner.exe")) pid = pe.th32ProcessID;
    CloseHandle(snap);
    if (!pid) { printf("no SnowRunner.exe\n"); return 2; }
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | (changes.empty() ? 0 : PROCESS_VM_WRITE | PROCESS_VM_OPERATION), FALSE, pid);
    if (!proc) { printf("OpenProcess failed (%lu)\n", GetLastError()); return 2; }
    printf("pid %lu\n", pid);

    const BYTE sig[8] = { 0x00, 0x00, 0x20, 0x41, 0xCD, 0xCC, 0x4C, 0x3D }; // 10.0f, 0.05f at +0xA8
    std::vector<BYTE> buf;
    int found = 0, written = 0;
    MEMORY_BASIC_INFORMATION mbi;
    for (BYTE *a = nullptr; VirtualQueryEx(proc, a, &mbi, sizeof mbi) == sizeof mbi; a = (BYTE *)mbi.BaseAddress + mbi.RegionSize)
    {
        if (mbi.State != MEM_COMMIT || mbi.Type != MEM_PRIVATE || (mbi.Protect & 0xFF) != PAGE_READWRITE || (mbi.Protect & PAGE_GUARD)) continue;
        buf.resize(mbi.RegionSize);
        SIZE_T got = 0;
        if (!ReadProcessMemory(proc, mbi.BaseAddress, buf.data(), mbi.RegionSize, &got) || got < 0x100) continue;
        for (size_t i = 0xA8; i + 0x30 <= got; i += 8)
        {
            if (memcmp(buf.data() + i, sig, 8)) continue;
            const BYTE *w = buf.data() + i - 0xA8;
            BYTE *addr = (BYTE *)mbi.BaseAddress + i - 0xA8;
            const float soft = F(w, 0x78), sub = F(w, 0xA0), asph = F(w, 0xA4), mass = F(w, 0xB0), off = F(w, 0xB4);
            if (soft != F(w, 0x7C) || !(soft >= 0.0f && soft <= 1.0f) || !(sub >= 0.0f && sub <= 100.0f) || !(asph > 0.0f && asph <= 100.0f) || !(mass > 0.0f)) continue;
            if (wantRadius >= 0 && fabsf(F(w, 0x94) - wantRadius) > 0.002f) continue;
            if (wantSoft >= 0 && fabsf(soft - wantSoft) > 0.002f) continue;
            found++;
            printf("wheel %p  soft %.3f  sub %.3f  asphalt %.3f  mass %.1f  offset %.4f  width %.4f  radius %.4f  flag %u  ignoreIce %u  +20 %p\n",
                   (void *)addr, soft, sub, asph, mass, off, F(w, 0x90), F(w, 0x94), w[0xD0], w[0xD1], (void *)*(const uint64_t *)(w + 0x20));
            if (probe)
            {
                // +0x18 = the wheel's Havok body (0x2D0 bytes), its shape pointer at body+0x20 (the collidable's first member)
                BYTE body[0x2D0] = {}, shape[0x70] = {};
                const uint64_t pb = *(const uint64_t *)(w + 0x18);
                if (ReadProcessMemory(proc, (void *)pb, body, sizeof body, nullptr))
                {
                    const uint64_t ps = *(const uint64_t *)(body + 0x20);
                    printf("    body %p  shape %p", (void *)pb, (void *)ps);
                    if (ReadProcessMemory(proc, (void *)ps, shape, sizeof shape, nullptr))
                        printf("  shape vtable %llx  +20 %.4f  +28 %.4f  +2C %.4f  A(%.3f %.3f %.3f) B(%.3f %.3f %.3f)", *(const uint64_t *)shape,
                               F(shape, 0x20), F(shape, 0x28), F(shape, 0x2C), F(shape, 0x30), F(shape, 0x34), F(shape, 0x38), F(shape, 0x40), F(shape, 0x44), F(shape, 0x48));
                    printf("\n    body floats 0.3..20:");
                    for (size_t o = 0; o < sizeof body; o += 4) if (F(body, o) >= 0.3f && F(body, o) <= 20.0f && F(body, o) != 1.0f) printf("  +%zX %.3f", o, F(body, o));
                    printf("\n");
                }
                // +0x28 = a 0x190-byte object the setup makes right after the body
                BYTE obj[0x190] = {};
                const uint64_t po = *(const uint64_t *)(w + 0x28);
                if (ReadProcessMemory(proc, (void *)po, obj, sizeof obj, nullptr))
                {
                    printf("    +28 object %p vtable %llx, floats 0.001..1000:", (void *)po, *(const uint64_t *)obj);
                    for (size_t o = 8; o < sizeof obj; o += 4) if (F(obj, o) >= 0.001f && F(obj, o) <= 1000.0f) printf("  +%zX %.4f", o, F(obj, o));
                    printf("\n");
                }
                printf("    wheel floats:");
                // (as far as this region was read: a wheel at its very end has less than 0x130 bytes in the buffer)
                for (size_t o = 0x38; o < 0x130 && i - 0xA8 + o + 4 <= got; o += 4) if (F(w, o) >= 0.001f && F(w, o) <= 1000.0f) printf("  +%zX %.4f", o, F(w, o));
                printf("\n");
            }
            for (const Change &c : changes)
            {
                if (c.f->where)
                {
                    // body: BodyFriction at body+0xCC (the Havok material); cyl: the cylinder radius at shape+0x28
                    uint64_t p = *(const uint64_t *)(w + 0x18);
                    if (c.f->where == 2 && !ReadProcessMemory(proc, (void *)(p + 0x20), &p, 8, nullptr)) p = 0;
                    float old = 0;
                    bool ok = p && ReadProcessMemory(proc, (void *)(p + c.f->off), &old, 4, nullptr);
                    const float now = c.scale ? old * c.v : c.v;
                    ok = ok && WriteProcessMemory(proc, (void *)(p + c.f->off), &now, 4, nullptr);
                    printf("    %s %.4f -> %.4f %s\n", c.f->name, old, now, ok ? "written" : "WRITE FAILED");
                    written += ok;
                    continue;
                }
                const float old = F(w, c.f->off), now = c.scale ? old * c.v : c.v;
                bool ok = WriteProcessMemory(proc, addr + c.f->off, &now, 4, nullptr) != 0;
                if (ok && c.f->off2) ok = WriteProcessMemory(proc, addr + c.f->off2, &now, 4, nullptr) != 0;
                printf("    %s %.4f -> %.4f %s\n", c.f->name, old, now, ok ? "written" : "WRITE FAILED");
                written += ok;
            }
        }
    }
    printf("%d wheel objects, %d values written\n", found, written);
    CloseHandle(proc);
    return found ? 0 : 1;
}
