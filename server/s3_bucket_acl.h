
#include <string>
// #include <json.h>

class S3BucketACL {
  // TODO

public:
  std::string to_json();

  void from_json(std::string content);
};
