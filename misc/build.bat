@echo off

if not exist ..\data mkdir ..\data
if not exist ..\..\build mkdir ..\..\build

pushd ..\..\build
cl  -MTd -WL -Od -nologo -fp:fast -fp:except- -Gm- -EHsc -Zo -Oi -WX -W4 -wd4100 -wd4458 -FC -Zi -GS-^
    ..\hy3d\code\hy3d.cpp^
    -link -incremental:no -opt:ref user32.lib gdi32.lib d3d11.lib
popd