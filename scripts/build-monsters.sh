#!/bin/sh
# Cross-build in a private directory. No other checkout's sources are edited.
set -eu
project=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
work="$project/work/monsters"
pin=bc68fcd635a918e16019767acad67cc1d773dd6a
base="$work/base"
mkdir -p "$work" "$project/out"
if [ ! -d "$base/.git" ]; then
    git init -q "$base"
    git -C "$base" remote add origin https://github.com/w0ts/fm1-base.git
    git -C "$base" fetch --depth 1 origin "$pin"
    git -C "$base" checkout -q --detach FETCH_HEAD
fi
test "$(git -C "$base" rev-parse HEAD)" = "$pin"
if [ ! -d "$base/bsp/chip" ]; then
    git -C "$base" -c url.https://github.com/.insteadOf=git@github.com: submodule update --init bsp
fi
test "$(git -C "$base/bsp" rev-parse HEAD)" = fdd709fd0066b878b03c144776404d63811db13e
test -z "$(git -C "$base" status --porcelain --untracked-files=no)"
if [ -z "${JIELI_TOOLCHAIN:-}" ]; then
    version=4f631b31b-e575321
    archive="pi32v2-llvm-dropin-$version.tar.xz"
    test "$(uname -s)-$(uname -m)" = Linux-x86_64 || {
        echo "Provide JIELI_TOOLCHAIN for this host. Release builds use Linux x86-64." >&2
        exit 1
    }
    if [ ! -f "$work/$archive" ]; then
        curl -fL --retry 3 "https://github.com/w0ts/optimist/releases/download/toolchain-pi32v2-llvm-$version/$archive" -o "$work/$archive.part"
        mv "$work/$archive.part" "$work/$archive"
    fi
    (cd "$work" && echo "72519811be85041a3b1ba4ec4b71068897903955d73a2d1beb77559e3a6d8f12  $archive" | sha256sum -c - && tar -xf "$archive")
    export JIELI_TOOLCHAIN="$work/pi32v2-llvm-dropin-$version"
fi
if [ -z "${AC79_SDK:-}" ]; then
    (cd "$base" && sh tools/setup_toolchain.sh --only ac79-sdk)
    export AC79_SDK="$base/.toolchain/ac79-sdk"
fi
export SOURCE_DATE_EPOCH=1795651200
mkdir -p "$base/monster-keys"
cp "$project"/monster-keys/* "$base/monster-keys/"
python3 "$base/tools/build.py" "$base/monster-keys" --release 2.0.1
cp "$base/monster-keys/build/"*2.0.1.fwsc "$project/out/monster-keys.fwsc"
