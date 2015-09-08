
#include "s3_action.h"

class S3PutBucketAction : public S3Action {
  S3BucketMetadata* metadata;

public:
  S3PutBucketAction(S3RequestObject* req);

  void setup_steps();

}
