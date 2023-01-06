
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

```
$ ./repoint install
$ meson setup build
$ ninja -C build
```

Then either `ninja -C build install`, or something like

```
$ VAMP_PATH=./build sonic-visualiser
```

Instructions for other platforms to follow.

## Licence

Expressive Means is distributed under the GNU General Public License
(GPL). You can redistribute it and/or modify it under the terms of the
GPL; either version 2 of the License, or (at your option) any later
version. See the file COPYING for more information.
