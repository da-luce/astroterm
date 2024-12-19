# ✨ starsaver

![Test Status](https://github.com/da-luce/starsaver/actions/workflows/ci.yml/badge.svg)

Stellar magic, now in your terminal! ✨🪐 View the live position stars, planets, constellations, and more, all rendered right the command line—no telescope required!

<p align="center">
  <img src="./assets/moving.gif" alt="Image 1" width="45%" style="display:inline-block; margin-right: 10px;">
  <img src="./assets/screenshot.png" alt="Image 2" width="45%" style="display:inline-block;">
</p>

_<p align="center">Stars above Boston around 9 PM on December 18, 2024</p>_

- [✨ starsaver](#-starsaver)
  - [Building](#building)
    - [Requirements](#requirements)
    - [Installation](#installation)
  - [Usage](#usage)
    - [Features](#features)
    - [Options](#options)
    - [Example](#example)
  - [Development](#development)
    - [Known issues](#known-issues)
    - [Testing](#testing)
  - [Citations](#citations)
  - [Data Sources](#data-sources)

## Building

> Ncurses detection is spotty on some systems, and you may need to install
> [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) in order
> for Meson to find it. You may install it via [Homebrew](https://formulae.brew.sh/formula/ncurses) on macOS.

### Requirements

- Unix-like environment (Linux, macOS, WSL, etc.)
- C compiler
- [`ncurses`](https://invisible-island.net/ncurses/announce.html) library
- [`meson`](https://github.com/mesonbuild/meson) 1.4.0 or newer
- [`ninja`](https://github.com/ninja-build/ninja) 1.8.2 or newer
- Some common CLI tools (_these are checked for automatically during install_)
  - [`wget`](https://www.gnu.org/software/wget/) or [`curl`](https://curl.se/)
  - [`xxd`](https://linux.die.net/man/1/xxd)
  - [`sed`](https://www.gnu.org/software/sed/manual/sed.html)

### Installation

Clone the repository and enter the project directory:

```sh
git clone https://github.com/da-luce/starsaver && cd starsaver
```

Run the install script:

```sh
sh install.sh
```

You may now run the generated `./build/starsaver` binary or add the `starsaver` command system wide via `meson install -C build`. Pressing <kbd>q</kbd> or <kbd>ESC</kbd> will exit the display.

## Usage

### Features

- 🔭 **Customizable Sky View:** Choose any date, time, and location to explore past, present, or future celestial events
- 🎯 **Accurate Sky Rendering:** Displays moon, stars, and planets as precisely as ASCII allows
- 🌘 **Moon Phases:** Displays precise lunar phases in real-time
- 🌌 **Constellation Figures:** Renders detailed constellation shapes
- ⚡ **Performance Optimized:** Lightweight and fast ASCII rendering

### Options

The `--help` flag displays all supported options.

### Example

Say we wanted to view the sky at 5:00 AM (Eastern) on July 16, 1969—the morning
of the Apollo 11 launch at the Kennedy Space Center in Florida. We would run:

```sh
starsaver --latitude 28.573469 --longitude -80.651070 --datetime 1969-7-16T9:32:00
```

If we then wanted to display all stars with a magnitude brighter than or equal
to 5.0 and add color, we would add `--threshold 5.0 --color` as options.

If you simply want the current time, don't specify the `--datetime` option and
_starsaver_ will use the system time. For your current location, you will still
have to specify the `--lat` and `--long` options.

For more options and help run `starsaver -h` or `starsaver --help`.

> ℹ️ Use a tool like [LatLong](https://www.latlong.net/) to get your latitude and longitude.

> ℹ️ Star magnitudes decrease as apparent brightness increases, i.e. to show more stars, increase the threshold.

## Development

### [Known issues](https://github.com/da-luce/starsaver/issues)

### Testing

Run `meson test` within the build directory. To get a coverage report, subsequently run `ninja coverage`.

## Citations

Many thanks to the following resources, which were invaluable to the development of this project.

- [Map Projections-A Working Manual By John P. Snyder](https://pubs.usgs.gov/pp/1395/report.pdf)
- [Wikipedia](https://en.wikipedia.org)
- [Atractor](https://www.atractor.pt/index-_en.html)
- [Jon Voisey's Blog: Following Kepler](https://jonvoisey.net/blog/)
- [Celestial Programming: Greg Miller's Astronomy Programming Page](https://astrogreg.com/convert_ra_dec_to_alt_az.html)
- [Practical Astronomy with your Calculator by Peter Duffett-Smith](https://www.amazon.com/Practical-Astronomy-Calculator-Peter-Duffett-Smith/dp/0521356997)

## Data Sources

- Stars: [Yale Bright Star Catalog](http://tdc-www.harvard.edu/catalogs/bsc5.html)
- Star names: [IAU Star Names](https://www.iau.org/public/themes/naming_stars/)
- Constellation figures: [Stellarium](https://github.com/Stellarium/stellarium/blob/3c8d3c448f82848e9d8c1af307ec4cad20f2a9c0/skycultures/modern/constellationship.fab#L6) (Converted from [Hipparchus](https://heasarc.gsfc.nasa.gov/w3browse/all/hipparcos.html) to [BSC5](http://tdc-www.harvard.edu/catalogs/bsc5.html) indices using the [HYG Database](https://www.astronexus.com/projects/hyg)—see [convert_constellations.py](./scripts/convert_constellations.py))
- Planet orbital elements: [NASA Jet Propulsion Laboratory](https://ssd.jpl.nasa.gov/planets/approx_pos.html)
- Planet magnitudes: [Computing Apparent Planetary Magnitudes for The Astronomical Almanac](https://arxiv.org/abs/1808.01973)
