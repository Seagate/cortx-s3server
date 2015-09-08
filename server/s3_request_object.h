
#include <string>

/* libevhtp */
#include <evhtp.h>

enum class S3HttpVerb {
  HEAD = htp_method_HEAD,
  GET = htp_method_GET,
  PUT = htp_method_PUT,
  DELETE = htp_method_DELETE,
  POST = htp_method_POST
};

class S3RequestObject {
  evhtp_request_t *ev_req;
  std::string bucket_name;
  std::string object_name;
  // This info is fetched from auth service.
  std::string user_name;
  std::string user_id;  // Unique
  std::string account_name;
  std::string account_id;  // Unique

public:
  struct event_base* get_evbase() {
    this->ev_req->evbase;
  }
  S3HttpVerb http_verb();

  const char* c_get_full_path();

  std::string get_header_value();
  std::string get_host_header();

  void set_out_header_value(std::string& key, std::string& value);

  // Operation params.
  void set_bucket_name(std::string& name);
  std::string& get_bucket_name();
  void set_object_name(std::string& name);
  std::string& get_object_name();

  void set_user_name(std::string& name);
  std::string& get_user_name();
  void set_user_id(std::string& id);
  std::string& get_user_id();

  void set_account_id(std::string& id);
  std::string& get_account_id();

  // Response Helpers
  void respond_unsupported_api();

};
