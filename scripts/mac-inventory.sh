#!/bin/sh
# Read-only. No installs, device queries, identity or credentials.
set -eu
if [ "$(uname -s)" != Darwin ]; then echo "Run this inventory on your Mac."; exit 2; fi
sw_vers
echo "Shell architecture: $(uname -m)"
echo "Apple silicon support: $(sysctl -n hw.optional.arm64 2>/dev/null || echo unknown)"
echo "Rosetta translation: $(sysctl -in sysctl.proc_translated 2>/dev/null || true)"
xcode-select -p 2>/dev/null || echo "No selected Xcode command-line tools"
for tool in git cc make brew python3 node codex code gh emcc cargo; do
    path=$(command -v "$tool" || true)
    if [ -z "$path" ]; then echo "$tool absent from PATH"; continue; fi
    case "$path" in
        /usr/bin/git|/usr/bin/cc|/usr/bin/make|/usr/bin/python3)
            if ! xcode-select -p >/dev/null 2>&1; then echo "$tool $path (Apple shim; version unchecked)"; continue; fi;;
    esac
    echo "$tool $path $("$path" --version 2>/dev/null | head -n 1)"
done
