#!/bin/sh
# Pinned local cross-build. Usage: sh scripts/build.sh [baseline|four-part]
set -eu
project=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
kind=${1:-four-part}
case "$kind" in baseline|four-part) ;; *) echo "Choose baseline or four-part"; exit 2;; esac
src="$project/work/optimist"
pin=d1fbcf9389f41d0721c7e0df74cf3f530cc2ca42
mkdir -p "$project/work" "$project/out"
if [ ! -d "$src/.git" ]; then
    git init -q "$src"
    git -C "$src" remote add origin https://github.com/w0ts/optimist.git
    git -C "$src" fetch --depth 1 origin "$pin"
    git -C "$src" checkout -q --detach FETCH_HEAD
fi
cd "$src"
test "$(git rev-parse HEAD)" = "$pin" || { echo "Existing checkout has a different revision"; exit 1; }
git -c url.https://github.com/.insteadOf=git@github.com: submodule update --init --recursive
test -z "$(git status --porcelain --untracked-files=no)" || { echo "Upstream has tracked changes"; exit 1; }
python3 tools/optimist.py setup --yes --no-emu
if [ "$kind" = baseline ]; then
    set -- --profile mots-config
else
    set -- --config "$project/four-part.config"
fi
python3 tools/optimist.py build "$@" --summary "$project/out/$kind.json"
cp build/felucca.fwsc "$project/out/$kind.fwsc"
