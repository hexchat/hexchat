#!/bin/sh

rm -rf HexChat.app
rm -f *.app.zip

python $HOME/.local/bin/gtk-mac-bundler hexchat.bundle

echo "Compressing bundle"
#hdiutil create -format UDBZ -srcdir HexChat.app -quiet HexChat-2.9.6.1-$(git rev-parse --short master).dmg
zip -9rXq ./HexChat-$(git describe --tags).app.zip ./HexChat.app

