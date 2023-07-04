#!/bin/bash
# Requires a known-quantity input file. We use
#   "test-material/1953 Szeryng_Beethoven op. 61, 2nd mov, 43-44.wav"
#   (1456338 bytes, SHA-1 fbc7a2b23d332debea00143e07fc797ac5fea98d)

set -eu

input="test-material/1953 Szeryng_Beethoven op. 61, 2nd mov, 43-44.wav"

if [ ! -f "$input" ]; then
    echo "Input file $input not found"
    exit 2
fi

suffix=
#suffix=-pyin-imprecise

while read output ; do 
    echo "--- Testing $output"
    suffixed=$(echo "$output" | sed 's/:/'$suffix':/')
    VAMP_PATH=$(pwd)/build sonic-annotator -d "vamp:expressive-means:$suffixed" -w csv --csv-stdout --csv-omit-filename "$input" > /tmp/$$
    expected=scripts/regression-expected/"$(echo $output | sed s/:/_/g)"
#    cp /tmp/$$ "$expected"
    if diff -q /tmp/$$ "$expected" ; then
        echo "--- Passed"
    else
        sdiff -w 78 /tmp/$$ "$expected"
        echo "*** FAILED"
    fi
    echo
done <<EOF
    onsets:onsets
    articulation:summary
    pitch-vibrato:summary
    portamento:summary
EOF

