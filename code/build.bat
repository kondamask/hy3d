@echo off

if not exist ..\data mkdir ..\data
if not exist ..\..\build mkdir ..\..\build

pushd ..\..\build
cl  -nologo^
    -Zi^
    -Fe:hy3d^
    -fp:fast^
    -EHsc^
    ..\hy3d\code\*.cpp^
    user32.lib gdi32.lib d3d11.lib
popd