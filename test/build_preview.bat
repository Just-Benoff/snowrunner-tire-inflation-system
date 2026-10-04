@echo off
rem Builds out\test\panel_preview.exe: the pressure panel drawn without the game (see test\panel_preview.cpp). Needs
rem the full Dear ImGui sources (1.92; imgui.cpp and backends\imgui_impl_dx11.cpp): set IMGUI_SRC to their folder, or
rem put them in deps\imgui_src. x64, Visual Studio 2022.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || (echo vcvars64 failed & exit /b 1)
cd /d "%~dp0.."
if not defined IMGUI_SRC set IMGUI_SRC=deps\imgui_src
rem no brackets around this check: a folder name with brackets in it would end them early
if not exist "%IMGUI_SRC%\imgui.cpp" goto nosources
if not exist out\obj\preview\shim mkdir out\obj\preview\shim
if not exist out\test mkdir out\test
rem a source tree that includes a precompiled header named shared.h gets an empty one
if not exist out\obj\preview\shim\shared.h echo // empty> out\obj\preview\shim\shared.h
cl /nologo /O2 /W3 /EHsc /std:c++17 /utf-8 /MT /I out\obj\preview\shim /I "%IMGUI_SRC%" /I "%IMGUI_SRC%\backends" /I deps\stb /I src /Fo:out\obj\preview\ /Fe:out\test\panel_preview.exe test\panel_preview.cpp "%IMGUI_SRC%\imgui.cpp" "%IMGUI_SRC%\imgui_draw.cpp" "%IMGUI_SRC%\imgui_tables.cpp" "%IMGUI_SRC%\imgui_widgets.cpp" "%IMGUI_SRC%\backends\imgui_impl_dx11.cpp" /link d3d11.lib d3dcompiler.lib windowscodecs.lib ole32.lib || (echo BUILD FAILED & exit /b 1)
echo BUILD OK: out\test\panel_preview.exe
exit /b 0
:nosources
echo no Dear ImGui sources in "%IMGUI_SRC%": set IMGUI_SRC
exit /b 1
