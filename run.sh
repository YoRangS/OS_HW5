set -x

mkdir mnt

./jsonfs fs.json ./mnt
#./fuse_example ./mnt
#./fuse_example -d -f ./mnt
