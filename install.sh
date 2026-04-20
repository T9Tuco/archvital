#!/bin/bash
set -e

REPO="T9Tuco/archvital"
URL="https://github.com/$REPO/releases/download/latest/archvital-linux-x86_64.tar.gz"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

echo "downloading archvital..."
curl -fsSL "$URL" -o "$TMP/archvital.tar.gz"
tar -xzf "$TMP/archvital.tar.gz" -C "$TMP"

echo "installing..."
sudo install -Dm755 "$TMP/archvital"          /usr/local/bin/archvital
sudo install -Dm644 "$TMP/archvital.desktop"  /usr/share/applications/archvital.desktop
sudo install -Dm644 "$TMP/archvital.svg"      /usr/share/pixmaps/archvital.svg

echo "done. run: archvital"
