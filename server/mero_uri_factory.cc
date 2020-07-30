#include "mero_uri.h"
#include "s3_log.h"

MeroURI* MeroUriFactory::create_uri_object(
    MeroUriType uri_type, std::shared_ptr<MeroRequestObject> request) {
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "Entering\n");

  switch (uri_type) {
    case MeroUriType::path_style:
      s3_log(S3_LOG_DEBUG, request->get_request_id(),
             "Creating mero path_style object\n");
      return new MeroPathStyleURI(request);
    default:
      break;
  };
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "Exiting\n");
  return NULL;
}
