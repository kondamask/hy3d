@echo off

set COMPILER_FLAGS=-MTd -WL -Od -nologo -fp:fast -fp:except- -Gm- -EHsc -Zo -Oi -W4 -wd4100 -wd4458 -FC -Zi -GS-
set LINKER_FLAGS=-subsystem:windows -incremental:no -opt:ref user32.lib gdi32.lib

if not exist .\\build mkdir .\\build
pushd .\\build

del *.pdb
cl  %COMPILER_FLAGS% ..\\code\\hy3d_engine.cpp -Fmhy3d_engine.map -LD -link -EXPORT:UpdateAndRender
cl  %COMPILER_FLAGS% ..\\code\\win32_platform.cpp -Fmwin32_platform.map -Fehy3d -link %LINKER_FLAGS%
popd
