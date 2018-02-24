# simperium-c

A C-library for syncing data via [Simperium](https://simperium.com/).

## Status

This is very WIP, with APIs implemented within a test app. Currently, I've
verified that we can:
* Authenticate the app with Simperium
* Add items to buckets
* Remove items from buckets
* Get items from buckets

Much remains TODO, including (but not limited to):
* Any kind of change polling
* network condition handling
* a clean header files
* API comments
* cross-platform support
* A sane build system

At some point, this will also support the WebSocket API (but currently it's all
HTTP)

## Dependencies

All dependencies are included in the "3rdparty" directory. They are:

- libcurl
- jansson
- argtable3
