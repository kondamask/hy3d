@echo off

set COMPILER_FLAGS=-MTd -WL -Od -nologo -fp:fast -fp:except- -Gm- -EHsc -Zo -Oi -W4 -wd4100 -wd4458 -FC -Zi -GS-
set LINKER_FLAGS=-link -subsystem:windows -incremental:no -opt:ref user32.lib gdi32.lib

if not exist .\\data mkdir .\\data
if not exist .\\build mkdir .\\build
pushd .\\build

del *.pdb
cl  %COMPILER_FLAGS% ..\\code\\hy3d.cpp %LINKER_FLAGS%
popd