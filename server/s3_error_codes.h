
#define S3HttpSuccess200         EVHTP_RES_OK
#define S3HttpFailed400          EVHTP_RES_400
#define S3HttpFailed500          EVHTP_RES_500



enum class S3Error {
  InvalidBucketName,  // 400
  InvalidBucketState,  // 409 The request is not valid with the current state of the bucket
  NoSuchBucket,  // 404
  NoSuchBucketPolicy,  // 404
  BucketAlreadyExists,  // 409
  BucketAlreadyOwnedByYou,  // 409 - previous request to create the named bucket succeeded and you already own it
  TooManyBuckets,  // 400
  PermanentRedirect,  // 301
  UserKeyMustBeSpecified,  // 400 The bucket POST must contain the specified field name. If it is specified, check the order of the fields.
}
