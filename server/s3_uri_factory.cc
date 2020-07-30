#include "s3_uri.h"
#include "s3_log.h"

S3URI* S3UriFactory::create_uri_object(
    S3UriType uri_type, std::shared_ptr<S3RequestObject> request) {
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "Entering\n");

  switch (uri_type) {
    case S3UriType::path_style:
      s3_log(S3_LOG_DEBUG, request->get_request_id(),
             "Creating path_style object\n");
      return new S3PathStyleURI(request);
    case S3UriType::virtual_host_style:
      s3_log(S3_LOG_DEBUG, request->get_request_id(),
             "Creating virtual_host_style object\n");
      return new S3VirtualHostStyleURI(request);
    default:
      break;
  };
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "Exiting\n");
  return NULL;
}
