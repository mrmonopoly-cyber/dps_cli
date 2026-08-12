# Dps Cli

A small cli to interact with a can bus using the dps protocol

# Building

To build the cli run the following command which uses the following lobrary [nob](https://github.com/tsoding/nob.h):

```sh

gcc nob.c -o nob && ./nob

```

If you need to alter the code or the build parameters you *do not* need to rebuild nob.
You only need execute nob again, it will update itself
