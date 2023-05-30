#!/bin/bash

set -eu

#case $(git status --porcelain --untracked-files=no) in
#    "") ;;
#    *) echo "ERROR: Current working copy has been modified - not proceeding"; e#xit 2;;
#esac

mydir=$(dirname "$0")

. "$mydir/../metadata.sh"

lib="expressive-means.dylib"
gatekeeper_key="Developer ID Application: Particular Programs Ltd (73F996B92S)"

./repoint install

tag="$(date +%s)"

for arch in arm64 x86_64 ; do
    builddir="build-${tag}-${arch}"
    BOOST_ROOT=/opt/boost arch -"$arch" meson setup "$builddir" -Dbuildtype=release -Dtests=disabled --cross-file deploy/macos/cross_"$arch".txt
    ninja -C "$builddir"
done

packdir="${full_condensed}-macos-${version}"

mkdir -p "$packdir"

lipo "build-${tag}-arm64/$lib" "build-${tag}-x86_64/$lib" -create -output "$packdir"/"$lib"

cp expressive-means.cat expressive-means.n3 README.md COPYING "$packdir/"

codesign -s "$gatekeeper_key" -fv --options runtime "$packdir/$lib"

rm -f "$packdir".zip
ditto -c -k "$packdir" "$packdir".zip

"$mydir"/notarize.sh "$packdir".zip "$full_ident"

