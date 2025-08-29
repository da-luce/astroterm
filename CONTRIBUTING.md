# Contributing

<!-- omit in toc -->
## Table of Contents

- [Contributing](#contributing)
  - [Building from Source](#building-from-source)
  - [Updating Data Sources](#updating-data-sources)
  - [New Releases](#new-releases)
  - [Miscellaneous](#miscellaneous)

## Building from Source

### Linux, macOS & WSL

#### Requirements

> [!IMPORTANT]
> When building, you must install the _development_ version of the runtime requirements, which provide the headers and libraries necessary for compiling and linking. These packages are typically marked with a `-dev` or `-devel` suffix.

- Unix-like environment (Linux, macOS, WSL, etc.)
- C compiler
- [`meson`](https://github.com/mesonbuild/meson) 1.4.0 or newer ([installation via python](https://mesonbuild.com/Quick-guide.html#installation-using-python) is recommended)
- [`ninja`](https://repology.org/project/ninja/versions) 1.8.2 or newer
- [`ncurses`](https://repology.org/project/ncurses/versions) library
- [`argtable2`](https://repology.org/project/argtable2/versions)
- Some common CLI tools
  - [`wget`](https://repology.org/project/wget/versions) or [`curl`](https://repology.org/project/curl/versions)
  - [`xxd`](https://repology.org/project/xxd/versions) (is also commonly packaged with [`vim`](https://repology.org/project/vim/versions))

> [!WARNING]
> `ncurses` and `argtable` detection is spotty on some systems, and you may need to install
> [`pkg-config`](https://repology.org/project/pkg-config/versions) in order
> for Meson to find them.

> [!TIP]
> See [`ci.yml`](./.github/workflows/ci.yml) for how `astroterm` is built and tested on Ubuntu via GitHub Actions.

#### Install

1. Clone the repository and enter the project directory:

  ```sh
  git clone https://github.com/da-luce/astroterm && cd astroterm
  ```

2. Download star data:

  ```sh
  curl -L -o data/bsc5 https://web.archive.org/web/20231007085824if_/http://tdc-www.harvard.edu/catalogs/BSC5
  ```

3. Build:

  ```sh
  meson setup build
  meson compile -C build
  ```

You may now run the generated `./build/astroterm` binary or add the `astroterm` command system-wide via `meson install -C build`. Pressing <kbd>q</kbd> or <kbd>ESC</kbd> will exit the display.

### Windows

> [!WARNING]
> Building on Windows is more involved than other platforms.

#### Requirements

- [Microsoft Visual C++](https://visualstudio.microsoft.com/vs/features/cplusplus/) (Other C compilers currently don't work)
- [`meson`](https://github.com/mesonbuild/meson) 1.4.0 or newer ([installation via python](https://mesonbuild.com/Quick-guide.html#installation-using-python) is recommended)
- [`ninja`](https://repology.org/project/ninja/versions) 1.8.2 or newer
- [`python`](https://www.python.org/downloads/) (for embedding data during build)
- [`pdcurses`](https://github.com/wmcbrine/PDCurses/tree/master/wincon)*
- [`argtable2`](https://github.com/jonathanmarvens/argtable2)*

> [!WARNING]
> *These libraries must be compiled locally and moved to where Meson expects them to be. See [`ci.yml`](./.github/workflows/ci.yml#L98) for how this is done.
>
#### Install

1. Clone the repository and enter the project directory:

  ```sh
  git clone https://github.com/da-luce/astroterm && cd astroterm
  ```

2. Download star data:

  ```sh
  curl -L -o data/bsc5 https://web.archive.org/web/20231007085824if_/http://tdc-www.harvard.edu/catalogs/BSC5
  ```

3. Build:

  ```sh
  meson setup build
  meson compile -C build
  ```

> [!TIP]
> Some steps must be done in the [Visual Studio Developer Command Prompt and Developer PowerShell](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022), so it's best to just do everything there.

---

## Updating Data Sources

- City data may be updated by running [`filter_cities.py`](./scripts/filter_cities.py)

```shell
python3 ./scripts/filter_cities.py
```

---

## New Releases

- This project follows
  - [Semantic Versioning](https://semver.org/)
  - [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/)

1. Create a new release commit, which bumps the program version in [`meson.build`](./meson.build). [Example](https://github.com/da-luce/astroterm/commit/6701a86f95dad77da6da737b4f5e2172a575c07a).

2. Tag this commit; this signals the [CI workflow](./.github/workflows/ci.yml) to publish the release.

```shell
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z
```

---

## Miscellaneous

### ASCII BSC5

You can use the ASCII version of the BSC5 star catalog by downloading and extracting the [gzip-compressed file](https://web.archive.org/web/20250114171002if_/http://tdc-www.harvard.edu/catalogs/ybsc5.gz) to `data/ybsc5` instead of `data/bsc5`.

### Testing

Run `meson test` within the build directory. To get a coverage report, subsequently run `ninja coverage`.
