#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <json.h>

#define MAX_FILE 128

static const char *filecontent = "I'm the content of the only file available there\n";

json_object * json_objs[MAX_FILE] ;

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (strcmp(path, "/file") == 0) {
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(filecontent);
    return 0;
  }

  return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {

  if (strcmp(path, "/") == 0) {

	  filler(buf, ".", NULL, 0);
	  filler(buf, "..", NULL, 0);
	  filler(buf, "file", NULL, 0);
	  return 0;
  }

  return -ENOENT ;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

  if (strcmp(path, "/file") == 0) {
    size_t len = strlen(filecontent);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
    return size;
  }

  return -ENOENT;
}

static int write_callback(const char *path,  const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    
  
  return -ENOENT;
}

static void create_by_json() {
  
}

static int init_callback(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  
}

static struct fuse_operations fuse_example_operations = {
  .init = init_callback,
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
  .write = write_callback,
};


void print_json (struct json_object * json) 
{
	int n = json_object_array_length(json);

	for ( int i = 0 ; i < n ; i++ ) {
		struct json_object * obj = json_object_array_get_idx(json, i);

		printf("{\n") ;
		json_object_object_foreach(obj, key, val) {
			if (strcmp(key, "inode") == 0) 
				printf("   inode: %d\n", (int) json_object_get_int(val)) ;

			if (strcmp(key, "type") == 0) 
				printf("   type: %s\n", (char *) json_object_get_string(val)) ;

			if (strcmp(key, "name" ) == 0)
				printf("   name: %s\n", (char *) json_object_get_string(val)) ;
			
			if (strcmp(key, "entries") == 0) {
				printf("   # entries: %d\n", json_object_array_length(val)) ;
				for ( int j = 0 ; j < json_object_array_length(val) ; j++ ) {
                			struct json_object * entry = json_object_array_get_idx(val, j);
					json_object_object_foreach(entry, key2, val2) {
						if(strcmp(key2, "name") == 0)
							printf("   \tname: %s\n", (char *) json_object_get_string(val2));
						if(strcmp(key2, "inode") == 0)
							printf("   \tinode: %d\n", (int) json_object_get_int(val2));
					}
				}
			}
		}
		printf("}\n") ;
	}
}

void init_inode(struct json_object * json) {
  int n = json_object_array_length(json) ;

  for ( int i = 0 ; i < n ; i++ ) {
    struct json_object * obj = json_object_array_get_idx(json, i) ;
    json_object_object_foreach(obj, key, val) {
      if (strcmp(key, "inode") == 0) {  
        json_objs[(int)json_object_get_int(val)] = obj ;
        break;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  char **new_argv = (char **)malloc(sizeof(char *) * (argc-1));
  int i;
  
  new_argv[0] = (char *)malloc(strlen(argv[0]) + 1);
  strcpy(new_argv[0], argv[0]);
  char * path;
  for(i = 2; i < argc; i++) {
    new_argv[i-1] = (char *)malloc(strlen(argv[i]) + 1);
    strcpy(new_argv[i-1], argv[i]);
    if(argv[i][0] != '-') {
      path = (char *)malloc(sizeof(char) * strlen(argv[i]) + 1);
      strcpy(path, argv[i]);
    }
  }

  struct json_object * fs_json = json_object_from_file(argv[1]) ;
  print_json(fs_json) ;
  init_inode(fs_json) ;
  json_object_put(fs_json) ;
  free(path);

  int ret = fuse_main(argc-1, new_argv, &fuse_example_operations, NULL) ;

  for(i = 0; i < argc - 1; i++) {
    free(new_argv[i]);
  }
  free(new_argv);

  return ret;
}
