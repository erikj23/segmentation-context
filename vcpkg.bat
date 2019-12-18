
@echo off
cmd /c cd ..\ && git clone https://github.com/Microsoft/vcpkg.git && .\bootstrap-vcpkg.bat && .\vcpkg integrate install && .\vcpkg install cpprestsdk:x64-windows
