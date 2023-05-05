#!/bin/bash

all_info=$(meson introspect --projectinfo meson.build -i)

version=$(echo "$all_info" | grep '"version"' | sed -e 's/^.*: "//' -e 's/".*$//')

case "$version" in
    [0-9].[0-9]) version="$version".0 ;;
    [0-9].[0-9].[0-9]) v="$version" ;;
    *) echo "## Error: Version $version is neither two- nor three-part number" ; exit 2;;
esac

full_name=$(echo "$all_info" | grep '"descriptive_name"' | sed -e 's/^.*: "//' -e 's/".*$//')

full_app="$full_name.app"
full_ident="uk.co.particularprograms.$(echo $full_name | sed 's/ //g')"
full_versioned="$full_name $version"
full_condensed=$(echo "$full_name" | sed 's/ //g')
full_kebab=$(echo "$full_name" | tr '[A-Z]' '[a-z]' | sed 's/ /-/g')
full_dmg="$full_versioned.dmg"
full_pkg="${full_condensed}_$version.pkg"
