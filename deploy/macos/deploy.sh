#!/bin/bash

set -eu

case $(git status --porcelain --untracked-files=no) in
    "") ;;
    *) echo "ERROR: Current working copy has been modified - not proceeding"; exit 2;;
esac

mydir=$(dirname "$0")

. "$mydir/../metadata.sh"

lib="expressive-means.dylib"
gatekeeper_key="Developer ID Application: Particular Programs Ltd (73F996B92S)"

mkdir -p packages
echo

tag="$(date +%s)"

for arch in arm64 x86_64 ; do
    builddir="build-${tag}-${arch}"
    meson setup "$builddir" -Dbuildtype=release --cross-file deploy/macos/cross_"$arch".txt
    ninja -C "$builddir"
done

packdir="${full_condensed}-macos-${version}"

mkdir -p "$packdir"

lipo "build-${tag}-arm64/$lib" "build-${tag}-x86_64/$lib" -create -output "$packdir"/"$lib"

cp expressive-means.cat README.md COPYING "$packdir/"

codesign -s "$gatekeeper_key" -fv --options runtime "$packdir/$lib"

rm -f "$packdir".zip
ditto -c -k "$packdir" "$packdir".zip

"$mydir"/notarize.sh "$packdir".zip "$full_ident"

