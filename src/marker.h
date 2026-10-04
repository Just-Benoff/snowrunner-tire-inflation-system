// marker.h: one text that both of this mod's files carry, version.dll and TirePressure.asi. An installer tells them
// from another mod's files of the same names by a byte search for it. Nothing reads it while the game runs, so the
// linker is told to keep it. Include it in one source file of each binary.
#pragma once
extern "C" const char kTireInflationSystemFile[] = "SnowRunner Tire Inflation System";
#pragma comment(linker, "/include:kTireInflationSystemFile")
