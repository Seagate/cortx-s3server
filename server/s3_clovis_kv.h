

// Clovis simplified interface.

EXTERN_C_BLOCK_BEGIN

typedef void (*clovis_kv_rw_callback)(int op_status, struct s3_e_kv_pair *kv, void* context);

struct s3_e_kv_pair {
  char *entity;
  size_t entity_length;
  char *key;
  size_t key_length;
  char *value;
  size_t value_length;
};

void free_s3_e_kv_pair(struct s3_e_kv_pair object);

struct s3_clovis_kv_ctx {
  void* app_ctx,
  clovis_kv_rw_callback callback
};

EXTERN_C_BLOCK_END

void s3_get_key_val(std::string& entity, std::string& key_name, clovis_kv_rw_callback callback, void* context);

void s3_put_key_val(std::string& entity, std::string& key_name, std::string& value, clovis_kv_rw_callback callback, void* context);

void s3_delete_key_val(std::string& entity, std::string& key_name, clovis_kv_rw_callback callback, void* context);

void s3_next_key_val(std::string& entity, std::string& key_name, int count, clovis_kv_rw_callback callback, void* context);
