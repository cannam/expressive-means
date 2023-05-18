#!/bin/bash

set -eu

mydir=$(dirname "$0")

tag=`git tag --list --sort=creatordate | grep '^v' | tail -1 | awk '{ print $1; }'`

v=`echo "$tag" | sed 's/v//' | sed 's/_.*$//'`

echo -n "Package up source code for version $v from tag $tag [Yn] ? "
read yn
case "$yn" in "") ;; [Yy]) ;; *) exit 3;; esac
echo "Proceeding"

case $(git status --porcelain --untracked-files=no) in
    "") ;;
    *) echo "ERROR: Current working copy has been modified - not proceeding"; exit 2;;
esac

echo
echo -n "Packaging up version $v from tag $tag... "

current=$(git rev-parse --short HEAD)

mkdir -p packages

git checkout "$tag"

./repoint archive "$(pwd)"/packages/ExpressiveMeans-source-"$v".tar.gz --exclude ext/pyin/testdata ext/pyin/test/regression ext/pyin/evaluation ext/vamp-plugin-sdk/test/expected .gitignore .github deploy scripts

git checkout "$current"

echo Done
echo
