# HDR Image Viewer

Small command-line tool that reads custom HDF HDR images, applies exposure and tone mapping, and writes a JPEG preview for quick inspection.

## What the project does

HDR Image Viewer loads `.hdf` images, converts the encoded RGBE pixels into linear floating-point RGB data, applies an exposure adjustment, tone maps the result with either Reinhard or ACES, and exports the final image as JPEG.

It is intentionally lightweight and ships with its image dependencies already vendored in the repository, so the project can be built without downloading extra libraries.

## Why the project is useful

- Quickly preview HDR samples from the command line.
- Compare Reinhard and ACES tone mapping with a single flag.
- Keep the full image pipeline in one small C program.
- Use the sample images in [images/](images/) to test changes immediately.

## How to get started

### Prerequisites

- GCC or Clang
- `make` or CMake
- A standard C toolchain with the math library available

The project already includes the required image headers in [include/](include/), so no extra package installation is needed for the runtime image code.

### Build with Make

```sh
make
```

This creates `output/hdrvis` on Unix-like systems.

### Build with CMake

```sh
cmake -S . -B build
cmake --build build
```

This produces the `transmutator` executable inside [build/](build/).

### Run the viewer

```sh
./output/hdrvis images/cathedral.hdf 0 2.2 1
```

Argument order:

1. Input `.hdf` file
2. Exposure stop
3. Gamma value
4. Tone mapping algorithm: `1` for Reinhard, `2` for ACES

Example with ACES tone mapping:

```sh
./output/hdrvis images/cathedral.hdf 1 2.2 2
```

The output JPEG is written to `output/<input>_<algorithm>.jpg`, for example `output/cathedral_Reinhard.jpg`.

## Where to get help

Start with the implementation in [main.c](main.c) to see the image format, command-line arguments, and processing pipeline.

For build-related details, check [Makefile](Makefile) and [CMakeLists.txt](CMakeLists.txt).

The sample inputs in [images/](images/) are useful for validating changes and reproducing output quickly.

## Maintainers and Contributions

Developed by:
[@achemello](https://github.com/achemello), [@brunopedralli](https://github.com/brunopedralli)

Maintainer: 
[@brunopedralli](https://github.com/brunopedralli)
