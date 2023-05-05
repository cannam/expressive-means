
# Expressive Means Vamp Plugins

* Method by Frithjof Vollmer, Stuttgart University of Music and the Performing Arts (Germany)
* Code by Chris Cannam, Particular Programs Ltd (London, UK)
* Makes use of "pYIN" by Matthias Mauch and of Tilo Haehnel's "vibratoanalyse.R" method

## To install from a pre-built package

Installation is the same as for other individual Vamp plugin libraries:

 * On Windows, copy `expressive-means.dll` and `expressive-means.cat`
   to the folder `C:\Program Files\Vamp Plugins\`. (Note that only
   64-bit versions of Windows and 64-bit plugin hosts are supported)

 * On a Mac, copy `expressive-means.dylib` and `expressive-means.cat`
   to the folder `/Library/Audio/Plug-Ins/Vamp` or
   `$HOME/Library/Audio/Plug-Ins/Vamp`

 * On Linux, copy `expressive-means.so` and `expressive-means.cat` to
   the folder `/usr/local/lib/vamp` or `$HOME/vamp`

In each case, if the destination folder doesn't already exist, create
it as a new folder first.

See the general Vamp [How to Install](https://vamp-plugins.org/download.html#install) guide for more information.

## To build from source code

### Prerequisites

There are several dependencies needed to build the plugins:

 * C++ compiler and typical build tools

 * Both Git and [Mercurial](https://www.mercurial-scm.org/) (the latter
   for pYIN code)

 * Any Standard ML compiler supported by
   [Repoint](https://github.com/cannam/repoint)
   ([Poly/ML](http://polyml.org), [SML/NJ](http://smlnj.org), or, on
   non-Windows platforms only, [MLton](http://mlton.org))

 * The [Meson](https://mesonbuild.com/) build system
 
 * The [Boost](https://www.boost.org/) Math library (used by pYIN)

None of these are required in order to use a plugin once it has been
built.

### Linux

Install the above-listed prerequisites using the distro package
manager, e.g. for Ubuntu try

```
$ sudo apt-get install -y build-essential git mercurial mlton meson ninja-build libboost-math-dev
```

then

```
$ ./repoint install
$ meson setup build
$ ninja -C build
```

Then either `ninja -C build install`, or try something like

```
$ VAMP_PATH=./build sonic-visualiser
```

### macOS

This is easiest if you use Homebrew. You also need the Xcode
command-line tools.

```
$ brew install mercurial polyml meson ninja boost
$ ./repoint install
$ meson setup build
$ ninja -C build
```

Then try something like

```
$ VAMP_PATH=./build sonic-visualiser
```

Instructions for other platforms to follow.

## Automated continuous integration builds

 * Linux CI build: [![Build Status](https://github.com/cannam/expressive-means/workflows/Linux%20CI/badge.svg)](https://github.com/cannam/expressive-means/actions?query=workflow%3A%22Linux+CI%22)
 * macOS CI build: [![Build Status](https://github.com/cannam/expressive-means/workflows/macOS%20CI/badge.svg)](https://github.com/cannam/expressive-means/actions?query=workflow%3A%22macOS+CI%22)

Note that each of these builds produces a compiled plugin as an
artifact, and you can download them for testing if you wish. To do so,
click on the build badge above for the platform you want, then on the
run you want to obtain the build from (typically the most recent one,
listed at the top of that page), then scroll down to Artifacts and
click on the one called `plugin`. Unzip the downloaded archive and
copy its contents into your Vamp Plugins folder.

## Licence

Expressive Means is distributed under the GNU General Public License
(GPL). You can redistribute it and/or modify it under the terms of the
GPL; either version 2 of the License, or (at your option) any later
version. See the file COPYING for more information.
