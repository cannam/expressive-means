
# Expressive Means Vamp Plugins

* Method by Frithjof Vollmer, HMDK Stuttgart
* Code by Chris Cannam, Particular Programs Ltd
* Makes use of pYIN by Matthias Mauch

## To build:

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
