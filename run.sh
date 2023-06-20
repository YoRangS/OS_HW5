set -x

mkdir mnt

fusermount ./mnt

#./jsonfs fs.json ./mnt
./jsonfs fs.json -d -f ./mnt
#./fuse_example ./mnt
#./fuse_example -d -f ./mnt
