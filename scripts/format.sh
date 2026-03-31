#!/bin/sh

# Directories to exclude
EXCLUDES="./third_party ./tests/third_party ./data ./build"

# Build the exclude arguments for find
EXCLUDE_ARGS=""
for dir in $EXCLUDES; do
  EXCLUDE_ARGS="$EXCLUDE_ARGS ! -path \"$dir\" ! -path \"$dir/*\""
done

# Output clang-format version
echo "Using clang-format version:"
clang-format --version

# Check if --check flag is provided
CHECK_MODE=0
if [ "$1" = "--check" ]; then
  CHECK_MODE=1
fi

# Find all C and header files while excluding specified directories
FILES=$(eval "find . \( -name '*.c' -o -name '*.h' \) $EXCLUDE_ARGS")

if [ $CHECK_MODE -eq 1 ]; then
  echo "Running clang-format in check mode..."
  # Dry-run and error on unformatted files
  clang-format --dry-run --Werror --style=file --verbose $FILES
  if [ $? -ne 0 ]; then
    echo "Some files are not properly formatted. Please fix the formatting."
    exit 1
  else
    echo "All files are properly formatted."
  fi
else
  echo "Running clang-format to fix formatting..."
  # Apply formatting in-place
  clang-format --style=file -i --verbose $FILES
  echo "Formatting complete."
fi
