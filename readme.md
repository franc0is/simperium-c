# simperium-c

A C-library for syncing data via [Simperium](https://simperium.com/).

## Status

This is very WIP, with APIs implemented within a test app. Currently, I've
verified that we can:
* Authenticate the app with Simperium
* Add items to buckets
* Remove items from buckets
* Get items from buckets
* Get all items in a bucket
* Poll bucket for changes

Most of those things can be done either over HTTP or WebSockets

Much remains TODO, including (but not limited to):
* diff resolving (see [jsondiff-c](https://github.com/franc0is/jsondiff-c))
* network condition handling
* a clean header files
* API comments
* cross-platform support
* A sane build system

## Dependencies

All dependencies are included in the "3rdparty" directory. They are:

- libcurl
- jansson
- argtable3
