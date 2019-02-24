
internal b32
linux_load_entire_file(const char *path, void **data, u64 *data_size) {
  b32 result = 0;
  
  if(data && data_size) {
    FILE *file = fopen(path, "rb");
    if(file) {
      fseek(file, 0, SEEK_END);
      *data_size = (u64)ftell(file);
      rewind(file);
      
      *data = malloc(*data_size);
      if(*data) {
        fread(*data, *data_size, 1, file);
        result = 1;
      }
      
      fclose(file);
    }
  }
  
  return result;
}

internal b32
linux_load_entire_file_from_read_data(const char *path, void **data, u64 *data_size) {
  b32 result = 0;
  // NOTE(rjf): "path" should be passed relative to the game's
  //            data folder, so we'll need to create the "real"
  //            path to load from.
  char real_path[512] = {0};
  snprintf(real_path, sizeof(real_path), "%s%s", global_read_data_path, path);
  result = linux_load_entire_file(real_path, data, data_size);
  return result;
}

internal b32
linux_load_entire_file_from_write_data(const char *path, void **data, u64 *data_size) {
  b32 result = 0;
  char real_path[512] = {0};
  snprintf(real_path, sizeof(real_path), "%s%s", global_write_data_path, path);
  result = linux_load_entire_file(real_path, data, data_size);
  return result;
}

internal b32
linux_write_to_file(const char *path, void *data, u64 data_size) {
  b32 result = 0;
  
  if(path && data && data_size) {
    char data_path[512] = {0};
    snprintf(data_path, sizeof(data_path), "%s%s", global_write_data_path, path);
    FILE *file = fopen(data_path, "w+");
    if(file) {
      fwrite(data, data_size, 1, file);
      result = 1;
      fclose(file);
    }
  }
  
  return result;
}