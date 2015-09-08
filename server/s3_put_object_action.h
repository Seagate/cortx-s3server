
#include "s3_action.h"

class S3PutObjectAction : public S3Action {
  S3ObjectMetadata* metadata;

public:
  S3PutObjectAction(S3RequestObject* req);

  void setup_steps();

  void read_metadata();
  void write_metadata();
  void write_object();

}
