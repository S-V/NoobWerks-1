"R:/testbed/Binaries/x86/shaderc.exe" ^
-f "R:/testbed/Assets/shaders/bgfx/imgui/fs_ocornut_imgui.sc" ^
-i "R:/testbed/Assets/shaders/bgfx/common;R:/testbed/Assets/shaders/bgfx/;" ^
-o "R:/fs_ocornut_imgui.sc.bin" ^
--platform "windows" ^
--type "fragment" ^
--varyingdef "R:/testbed/Assets/shaders/bgfx/imgui/varying.def.sc" ^
--debug ^
--disasm ^
-O3
::--verbose
pause
