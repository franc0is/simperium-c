# simperium-c

A C-library for syncing data via [Simperium](https://simperium.com/).

## Status

This is very WIP, with APIs implemented within a test app. Currently, I've
verified that we can:
* Authenticate the app with Simperium
* Add items to buckets
* Remove items from buckets
* Get items from buckets

Any kind of change polling, network condition handling, and a clean header files
are still TODO.

## Dependencies

- libcurl
- jansson
- argtable3
