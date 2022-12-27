#!/bin/sh
. $PWD/meta/environment.sh
meson setup build --cross-file $PWD/meta/cross/aarch64-macos.ini
