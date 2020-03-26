# mulle-mmap

An easy to use cross-platform C memory mapping library with a MIT license.

This is based on [https://github.com/mandreyel/mio](), which is a C++ header
only library. This library uses traditional `.h`/`.c` separation.
It has no shared pointer functionality.

Use Objective-C if you need retain counts.


## How to add

mulle-sde dependency add --c \
                         --github mulle-core \
                         --marks no-link,no-singlephase \
                         mulle-mmap

## How to build

This is a [mulle-sde](https://mulle-sde.github.io/) project.

To build it use `mulle-sde craft`.
