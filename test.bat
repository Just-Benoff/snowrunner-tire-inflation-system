@echo off
rem Runs the offline tests build.bat made: the loader on its own, the loader with a second copy of itself as its
rem chain file, and the mod's own checks (ini round trip, flattening and gear numbers, the write breakpoint probe).
cd /d "%~dp0"
set failed=0
out\test\loader_test.exe || set failed=1
out\test\chain\loader_test.exe || set failed=1
out\test\probe_test.exe || set failed=1
if %failed%==1 (echo TESTS FAILED & exit /b 1)
echo TESTS OK
