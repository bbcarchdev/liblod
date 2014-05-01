# liblod
## A client for Linked Open Data

`liblod` is a client library for consuming Linked Open Data published on
web servers. It's a thin wrapper around [librdf](http://librdf.org),
[libcurl](http://curl.haxx.se/libcurl/), and [libxml2](http://xmlsoft.org).

## Building Twine from Git

Twine uses `autoconf`, `libtool` and `automake` to build.

Once the dependencies are in place, you can check out and built Twine with:

```shell
$ git clone git://github.com/bbcarchdev/liblod.git
$ cd liblod
$ git submodule update --init --recursive
$ autoreconf -i
$ ./configure [--prefix=/optional/installation/prefix]
$ make
$ sudo make install
```

`liblod` includes a small command-line utility for interactively consuming
Linked Open Data, `lod-util`. When you run `lod-util`, either type a URI to
resolve it and print the discovered data, or type `.help` for a list of
available commands.

