# Coro

A C++ coroutine library, built upon coroutine support introduced in C++20.

## Author

Martin Beran

<martin@mber.cz>

## License

This software is available under the terms of BSD 2-Clause License, see
file [LICENSE.md](LICENSE.html).

## Development status

_This is a work in progress. It is currently usable for experiments with
coroutines and for learning how coroutines work. It is incomplete, probably
buggy, and not suitable for any production use._

## Repository structure

- `doc/` – documentation
- `src/` – source code
- `test/` – test programs

## Build

The library itself is header only.

Build test programs and basic (Markdown) documentation:

    mkdir build
    cd build
    cmake ..
    cmake --build . -j `nproc`

## [Documentation](html/index.html)

Documentation of Coro is generated by Doxygen from comments in source
code and from files in subdirectory `doc/` into build subdirectory
[html/](html/index.html) by command

    make doxygen
