
#ifdef __cplusplus
#define EXTERN_C_BLOCK_BEGIN    extern "C" {
#define EXTERN_C_BLOCK_END      }
#define EXTERN_C_FUNC           extern "C"
#else
#define EXTERN_C_BLOCK_BEGIN
#define EXTERN_C_BLOCK_END
#define EXTERN_C_FUNC
#endif


enum class S3AsyncOpStatus {
  unknown,
  inprogress,
  success,
  failed
};

enum class S3IOOpStatus {
  saved,
  failed
}
