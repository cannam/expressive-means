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

while read output ; do 
    echo "--- Testing $output"
    VAMP_PATH=./build sonic-annotator -d "vamp:expressive-means:$output" -w csv --csv-stdout --csv-omit-filename "$input" > /tmp/$$
    expected=scripts/regression-expected/"$(echo $output | sed s/:/_/g)"
    if sdiff -w 78 /tmp/$$ "$expected" ; then
        echo "--- Passed"
    else
        echo "*** FAILED"
    fi
    echo
done <<EOF
    onsets:onsets
    articulation:summary
    pitch-vibrato:summary
    portamento:summary
EOF

