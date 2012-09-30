#run this from mozilla-build\start-msvc11-x64.bat
#cd /c/mozilla-build/hexchat/libffi-3.0.11

./configure CC="$(pwd)/msvcc.sh -m64" LD=link CPP='cl -nologo -EP' CFLAGS='-O2' --build=x86_64-w64-mingw32
make clean
make
