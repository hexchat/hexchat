#run this from mozilla-build\start-msvc10.bat
#cd /c/mozilla-build/hexchat/libffi-3.0.11
#TODO: use own zlib build instead of mozilla-build one

./configure CC=$(pwd)/msvcc.sh LD=link CPP='cl -nologo -EP' CFLAGS='-O2' --build=i686-pc-mingw32
make clean
make
