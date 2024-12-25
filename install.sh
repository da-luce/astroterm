#!/bin/sh
 
SCRIPT_DIR="$(dirname "$0")" # directory where the script is located

check_dependencies() {
    for dep in "xxd" "sed" "meson" "ninja"; do
        if ! command -v "$dep" > /dev/null 2>&1; then
            echo "Error: $dep is not installed. Please install $dep to continue."
            exit 1
        fi
    done

    if ! (command -v curl > /dev/null 2>&1 || command -v wget > /dev/null 2>&1); then
        echo "Error: Neither curl nor wget is installed. Please install one of them to continue."
        exit 1
    fi

    echo "All dependencies are installed."
}

download_bsc5() {
    BSC5_URL="http://tdc-www.harvard.edu/catalogs/BSC5"
    BSC5_LOCATION="data/bsc5"
    DOWNLOAD_LOCATION="$SCRIPT_DIR/$BSC5_LOCATION"

    if [ -f "$DOWNLOAD_LOCATION" ]; then
        echo "BSC5 file already exists at $DOWNLOAD_LOCATION. Skipping download."
        return
    fi
    if command -v curl > /dev/null 2>&1; then
        echo "Using curl to download BSC5..."
        curl -L -o  "$DOWNLOAD_LOCATION" "$BSC5_URL"
    elif command -v wget > /dev/null 2>&1; then
        echo "Using wget to download BSC5..."
        wget -O "$DOWNLOAD_LOCATION" "$BSC5_URL"
    else
        echo "Error: Neither curl nor wget is installed. Please install one of them, or download BSC5 manually: $BSC5_URL"
        exit 1
    fi

    echo "Download completed successfully and saved to $DOWNLOAD_LOCATION"
}

build_with_meson() {
    BUILD_DIR="$SCRIPT_DIR/build"

    if [ "$GITHUB_ACTIONS" = "true" ]; then
        MESON_FLAGS=" --buildtype debugoptimized -Db_coverage=true"
        echo "Running as a GitHub Action. Adding flags: $MESON_FLAGS"
    else
        MESON_FLAGS=""
    fi

    # Create logs directory for capturing output
    LOG_DIR="$SCRIPT_DIR/log"
    mkdir -p "$LOG_DIR"

    CONFIGURE_LOG="$LOG_DIR/meson_configure.log"
    BUILD_LOG="$LOG_DIR/meson_build.log"

    # Run Meson configure step and capture errors
    echo "Configuring project with Meson..."
    if ! meson setup $MESON_FLAGS "$BUILD_DIR" "$SCRIPT_DIR" --wipe; then
        echo "Error: Meson configuration failed." >&2
        exit 1
    fi

    # Build step
    echo "Building project with Ninja..."
    if ! meson compile -C "$BUILD_DIR"; then
        echo "Error: Meson build failed." >&2
        exit 1
    fi

    echo "Build completed successfully!"
}


check_dependencies
download_bsc5
build_with_meson
