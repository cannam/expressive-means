
# Expressive Means Vamp Plugins

* Method by Frithjof Vollmer, HMDK Stuttgart
* Code by Chris Cannam, Particular Programs Ltd
* Makes use of pYIN by Matthias Mauch

## To build:

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
