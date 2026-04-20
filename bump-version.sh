#!/bin/bash
set -e

if [ -z "$1" ]; then
    echo "usage: ./bump-version.sh 1.2.0"
    exit 1
fi

VERSION="$1"

sed -i "s/project(archvital VERSION [0-9]*\.[0-9]*\.[0-9]*/project(archvital VERSION $VERSION/" CMakeLists.txt
sed -i "s/^pkgver=.*/pkgver=$VERSION/" PKGBUILD-stable
sed -i "s/^pkgrel=.*/pkgrel=1/" PKGBUILD-stable

git add CMakeLists.txt PKGBUILD-stable
git commit -m "bump: version to $VERSION"
git tag "v$VERSION"
git push origin main "v$VERSION"

echo "released v$VERSION"
