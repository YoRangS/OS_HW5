#define FUSE_USE_VERSION 31

#include <stdio.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <json.h>
#include <dirent.h>

#define MAX_FILE 128

static const char *filecontent = "I'm the content of the only file available there\n";

json_object * json_objs[MAX_FILE] ;
char root_path[1024];

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
    
  DIR* dir = opendir(path);
  if (dir == NULL) {
    return -errno;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    filler(buf, entry->d_name, NULL, 0);
  }

  closedir(dir);

  return 0;

}
static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
  
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    return -errno;
  }

  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (offset > file_size) {
    fclose(file);
    return 0;
  }
  if (offset + size > file_size) {
    size = file_size - offset;
  }

  fseek(file, offset, SEEK_SET);
  size_t bytes_read = fread(buf, 1, size, file);

  fclose(file);

  return bytes_read;
}

static int write_callback(const char *path,  const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  printf("w debug 0-1 %s\n", path);
  FILE *file = fopen(path, "w+");
  printf("w debug 0-2\n");
  if (!file) {
      printf("w debug 1\n");
      return -errno;
  }
  printf("w debug 0-3\n");

  if (fseek(file, offset, SEEK_SET) < 0) {
      printf("w debug 2\n");
      fclose(file);
      return -errno;
  }
  printf("w debug 0-4\n");

  printf("w debug 3\n");
  size_t bytes_written = fwrite(buf, 1, size, file);
  if (bytes_written < size) {
      fclose(file);
      return -errno;
  }
  printf("w debug 4\n");

  fclose(file);

  return bytes_written;
}

static int create_callback(const char *path, mode_t mode, struct fuse_file_info * fi) {
  printf("c debug\n");
  int res;

  res = open_callback(path, fi);
  if(res == -1) {
    printf("c debug 1-1\n");
    return -errno;
  }
  printf("c debug 2\n");

  fi->fh = res;
  printf("c debug 3 3 3\n");

  return 0;
}

static int mkdir_callback(const char *path, mode_t mode) {
  int res = mkdir(path, mode);
  if (res == -1) {
      return -errno;
  }
  return 0;
}

static int create_entries(struct json_object * entries, char* curr_path) {
  printf("create_entries gogogogo\n");
  for ( int i = 0 ; i < json_object_array_length(entries) ; i++ ) {
    struct json_object * entry = json_object_array_get_idx(entries, i);
    char * name;
    int inode;
    json_object_object_foreach(entry, key, val) {
      if(strcmp(key, "name") == 0) {
        name = (char *)malloc(strlen((char*)json_object_get_string(val)) + 1);
        strcpy(name, (char *) json_object_get_string(val));       
      }
      if(strcmp(key, "inode") == 0)
        inode = (int) json_object_get_int(val);
    }
    printf("debug\n"); 
    char path[1024];
    strcpy(path, curr_path);
    strcat(path, "/");
    strcat(path, name);
    printf("path is %s\n", path);

    json_object * _type;
    json_object_object_get_ex(json_objs[inode], "type", &_type);

    char type[4];
    int result;
    if (_type != NULL) {
      strcpy(type, (char *) json_object_get_string(_type));
    }
    printf("debug2\n"); 
    if (strcmp(type, "dir") == 0) {
      printf("debug2-1\n"); 
      int res = mkdir_callback(path, 0755);
      if (res != 0)
	return res;
      printf("debug2-2\n"); 
      json_object * dir_entry;
      json_object_object_get_ex(json_objs[inode], "entries", &dir_entry);
      create_entries(dir_entry, path);
      printf("debug2-3\n"); 
    }
    else if (strcmp(type, "reg") == 0) {
      printf("debug2-4\n"); 
      struct fuse_file_info fi;
      fi.flags = O_CREAT;
      printf("debug2-5\n"); 
      int res = create_callback(path, 0755, &fi);
      if (res != 0)
        return res;
      printf("debug2-6\n"); 
      char buf[4096];
      json_object * _buf;
      json_object_object_get_ex(json_objs[inode], "data", &_buf);
      printf("debug2-7\n"); 
      strcpy(buf, (char *) json_object_get_string(_buf));
      size_t size = strlen(buf);
      off_t offset = 0;

      printf("debug2-8\n"); 
      result = write_callback(path, buf, size, offset, &fi);
      if (result != size) {
        printf("debug2-8-1\n"); 
        return -EIO;
      }
      printf("debug2-9\n"); 
    }
    printf("debug3\n"); 
  }
  return 0;
}

static void * init_callback(struct fuse_conn_info *conn/*, struct fuse_config *cfg*/) {
  //struct json_object * root = json_objs[0];
  //printf("root:%p\tobjs[]:%p\n", root, json_objs[0]);
  printf("start!\n");
  //if (root == NULL) {
   // printf("NULL!!\n");
  //}
  printf("whyytyyyyyy\n");
  json_object_object_foreach(json_objs[0], key, val) {
    printf("create!\n");
    if (strcmp(key, "entries") == 0) {
      int res = create_entries(val, root_path);
      if (res != 0)
	return NULL; // need to manage error
    }
  }
  printf("end!\n");
  return NULL;
}

static struct fuse_operations fuse_example_operations = {
  .init = init_callback,
  .create = create_callback,
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
  .write = write_callback,
  .mkdir = mkdir_callback,
};


void print_json (struct json_object * json) 
{
	int n = json_object_array_length(json);
	printf("length: %d\n", n);

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
  for(i = 2; i < argc; i++) {
    new_argv[i-1] = (char *)malloc(strlen(argv[i]) + 1);
    strcpy(new_argv[i-1], argv[i]);
    if(argv[i][0] != '-') {
      strcpy(root_path, argv[i]);
    }
  }
  printf("rootpath : %s\n", root_path);

  struct json_object * fs_json = json_object_from_file(argv[1]) ;
  print_json(fs_json) ;
  init_inode(fs_json) ;

  printf("after init_inode!\n");
  int ret = fuse_main(argc-1, new_argv, &fuse_example_operations, NULL) ;

  json_object_put(fs_json) ;
  for(i = 0; i < argc - 1; i++) {
    free(new_argv[i]);
  }
  free(new_argv);

  return ret;
}
