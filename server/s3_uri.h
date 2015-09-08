
#include <string>

enum class S3OperationCode {
  none,  // Operation on current object.
  list,

  // Common Operations
  acl,

  // Bucket Operations
  location,
  policy,
  logging,
  lifecycle,
  cors,
  notification,
  replicaton,
  tagging,
  requestPayment,
  versioning,
  website,
  listuploads,

  // Object Operations
  initupload,
  partupload,
  completeupload,
  abortupload,
  multidelete
}

class S3URI {

protected:
  std::string full_path;

private:
  std::string bucket_name;
  std::string object_name;
  bool is_service_api;
  bool is_bucket_api;
  bool is_object_api;

public:
  S3URI();

  bool is_service_api();
  bool is_bucket_api();
  bool is_object_api();

  virtual std::string get_bucket_name() = 0;
  virtual std::string get_object_name() = 0;  // Object key

  virtual S3OperationCode get_operation_code() = 0;
};

class S3PathStyleURI : public S3URI {
public:
  S3PathStyleURI(char* uri);
  S3OperationCode get_operation_code();
};

class S3VirtualHostStyleURI : public S3URI {

public:
  S3VirtualHostStyleURI(std::string host_header, std::string uri);
  S3OperationCode get_operation_code();
};
