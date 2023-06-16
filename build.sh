set -x
gcc -Wall fuse-example.c $(pkg-config fuse json-c --cflags --libs) -o fuse_example -D_FILE_OFFSET_BITS=64
