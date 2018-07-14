#!/bin/sh
cd $(dirname $0)/..
rm -rf report
mkdir report
python -m gcovr -r . --html --html-details -o report/poly_vector.html --filter="include/.*" --html-title poly_vector --exclude-unreachable-branches
