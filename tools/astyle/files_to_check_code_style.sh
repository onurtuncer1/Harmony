#!/usr/bin/env bash
set -eu

PATTERN="-e ."

if [ $# -gt 0 ]; then
    PATTERN="$1"
fi

# Starting directory (default: current dir)
START_DIR="${1:-.}"

if [ -d "$START_DIR/build" ]; then
    # build directory exists, exclude it
    HEADER_DIRS=$(find "$START_DIR" -path "$START_DIR/build" -prune -o -type f -name '*.h' -exec dirname {} \; | sort -u)
else
    # build directory doesn't exist, just search normally
    HEADER_DIRS=$(find "$START_DIR" -type f -name '*.h' -exec dirname {} \; | sort -u)
fi

# Print the result
echo "Directories containing .h files:"
echo "$HEADER_DIRS"

exec find src $HEADER_DIRS test \
    \( -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) -print \) | grep $PATTERN
