```
 _      __   _
| |__  / /_ | |__
| '_ \| '_ \| '_ \
| |_) | (_) | |_) |
|_.__/ \___/|_.__/
```

[![Build Status](https://travis-ci.org/dimkr/b6b.svg?branch=master)](https://travis-ci.org/dimkr/b6b)

## Overview

b6b is a simple, dynamic, procedural and reflective scripting language inspired
by [Tcl](http://www.tcl.tk/), [Python](http://www.python.org/), Lisp and shell
scripting.

b6b wraps operating system mechanisms and interfaces such as sockets, files,
shell commands and even signals with a unified interface: the stream. I/O events
generated by streams of any kind can be managed through a single event loop.
This way, b6b enables rapid development of lightweight, asynchronous and
scalable, event-driven applications.

It is named so in admiration of the excellent Vincent Bach 6B trumpet
mouthpiece, which produces compact, yet lively and expressive tone with a
strong, solid core, that makes it a good choice for many playing situations. The
name was chosen with hope that these qualities will also be attributed to the
language and its implementation.

## Building

```bash
meson build
ninja -C build install
```

## Credits and Legal Information

b6b is free and unencumbered software released under the terms of the Apache
License Version 2.0; see COPYING for the license text. For a list of its authors
and contributors, see AUTHORS.

The ASCII art logo at the top was made using [FIGlet](http://www.figlet.org/).