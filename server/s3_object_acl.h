
#include <string>
// #include <json.h>

class S3ObjectACL {
  // TODO

public:
  std::string to_json();

  void from_json(std::string content);
};
