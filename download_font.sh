#!/bin/bash
# Download Font Awesome for icons
set -e
cd "$(dirname "$0")/fonts"
wget -O fa-solid-900.ttf "https://github.com/FortAwesome/Font-Awesome/raw/6.x/webfonts/fa-solid-900.ttf"
chmod 644 fa-solid-900.ttf
