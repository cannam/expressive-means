#!/bin/bash

set -e

vamp-plugin-tester --version >/dev/null || {
    echo "Unable to find vamp-plugin-tester in PATH"
    exit 2
}

set -u

VAMP_PATH=./build:. vamp-plugin-tester -a


