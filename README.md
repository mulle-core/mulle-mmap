# mulle-mmap

#### 🇧🇿 Memory mapped file access

An easy to use cross-platform C memory mapping library with a MIT license.

This is based on [//github.com/mandreyel/mio](), which is a C++ header
only library. This library uses traditional `.h`/`.c` separation.
It has no shared pointer functionality.

Use Objective-C, if you need retain counts.


### You are here

![Overview](overview.dot.svg)


## Add 

Use [mulle-sde](//github.com/mulle-sde) to add mulle-mmap to your project:

```
mulle-sde dependency add --c --github mulle-core mulle-mmap
```

## Install

### mulle-sde

Use [mulle-sde](//github.com/mulle-sde) to build and install mulle-mmap and all dependencies:

```
mulle-sde install --prefix /usr/local \
   //github.com/mulle-core/mulle-mmap/archive/latest.tar.gz
```

### Manual Installation

Install into `/usr/local`:

```
mkdir build 2> /dev/null
(
   cd build ;
   cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DCMAKE_PREFIX_PATH=/usr/local \
         -DCMAKE_BUILD_TYPE=Release .. ;
   make install
)
```

