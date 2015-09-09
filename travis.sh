#!/bin/bash
# script to setup install environment for travis

PWD=`pwd`
DIR="$PWD/node"
OS=$TRAVIS_OS_NAME

if [ "$NODE_VERSION" = "0.12" ]; then
	if [ "$OS" = "linux" ]; then
		echo "As of now, Linux 0.12 seems borked... Skipping!"
		exit 0
	fi
fi

if [ "$NODE_VERSION" = "4" ]; then
	VER="v4.0.0"
elif [ "$NODE_VERSION" = "0.10" ]; then
	VER="v0.10.40"
elif [ "$NODE_VERSION" = "0.12" ]; then
	VER="v0.12.7"
else
	echo "unknown node version"
	exit 1
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then
	OS="darwin"
fi

mkdir -p $DIR
URL=https://nodejs.org/dist/$VER/node-$VER-$OS-x64.tar.gz
echo "Downloading $URL"
curl $URL | tar xz --strip-components=1 -C $DIR

echo "export PATH=$DIR/bin:$PATH" >> ~/.profile
source ~/.profile
node --version
cd $PWD
rm -rf ~/.node-gyp
rm -rf ~/.npm
npm install npm -g
npm install node-gyp -g

if [ "`node --version`" != "$VER" ]; then
	echo "Node didn't match $VER as expected!"
	exit 1
fi

npm install
npm test

exit 0
