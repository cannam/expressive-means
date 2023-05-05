#!/bin/bash

# Previously:
#
# security add-generic-password -a "appstore@particularprograms.co.uk" \
#   -w "generated-app-password" -s "altool"
#
# (using app password from appleid.apple.com)

user="appstore@particularprograms.co.uk"

set -e

object="$1"
bundleid="$2"

if [ -z "$object" ] || [ -z "$bundleid" ] || [ -n "$3" ]; then
    echo "Usage: $0 <package> <bundleid>"
    echo "  e.g. $0 MyApplication-1.0.dmg com.example.MyApplication"
    exit 2
fi

set -u

if [ ! -f "$object" ]; then
    echo "File $object not found"
    exit 1
fi

echo
echo "Uploading for notarization..."

uuidfile=.notarization-uuid
statfile=.notarization-status
rm -f "$uuidfile" "$statfile"

xcrun altool --notarize-app \
    -f "$object" \
    --primary-bundle-id "$bundleid" \
    -u "$user" \
    -p @keychain:altool 2>&1 | tee "$uuidfile"

uuid=$(cat "$uuidfile" | grep RequestUUID | awk '{ print $3; }')

if [ -z "$uuid" ]; then
    echo
    echo "Failed (no UUID returned, check output)"
    exit 1
fi

echo "Done, UUID is $uuid"

echo
echo "Waiting and checking for completion..."

while true ; do
    sleep 30

    xcrun altool --notarization-info \
	"$uuid" \
	-u "$user" \
	-p @keychain:altool 2>&1 | tee "$statfile"

    if grep -q 'Package Approved' "$statfile"; then
	echo
	echo "Approved! Status output is:"
	cat "$statfile"
	break
    elif grep -q 'in progress' "$statfile" ; then
	echo
	echo "Still in progress... Status output is:"
	cat "$statfile"
	echo "Waiting..."
    else 
	echo
	echo "Failure or unknown status in output:"
	cat "$statfile"
	exit 2
    fi
done

case "$object" in
    *.dmg)
	echo
	echo "Stapling to disk image..."
	xcrun stapler staple "$object";;
    *.pkg)
	echo
	echo "Stapling to package..."
	xcrun stapler staple "$object";;
    *)
	echo
	echo "Object is not a disk image or package, not stapling";;
esac



