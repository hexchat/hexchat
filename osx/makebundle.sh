#!/bin/sh

if [ -z "$JHBUILD_PREFIX" ]; then
	echo "You must run this within a jhbuild shell."
	exit 1
fi

if [ ! -f $JHBUILD_PREFIX/bin/python ]; then
	echo "You must install python with jhbuild."
	exit 1
fi

rm -rf HexChat.app
rm -f *.app.zip

$JHBUILD_PREFIX/bin/python $HOME/.local/bin/gtk-mac-bundler hexchat.bundle

# These take up a lot of space in the bundle
echo "Cleaning up python files"
find ./HexChat.app/Contents/Resources/lib/python2.7 -name "*.pyc" -delete
find ./HexChat.app/Contents/Resources/lib/python2.7 -name "*.pyo" -delete

echo "Compressing bundle"
#hdiutil create -format UDBZ -srcdir HexChat.app -quiet HexChat-2.9.6.1-$(git rev-parse --short master).dmg
zip -9rXq ./HexChat-2.9.6.1-$(git rev-parse --short master).app.zip ./HexChat.app 
