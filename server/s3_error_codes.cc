
#include "s3_error_codes.h"

S3Error::S3Error(std::string error_code, std::string req_id, std::string res_key) : code(error_code), request_id(req_id), resource_key(res_key),
    details(S3ErrorMessages::get_instance()->get_details(error_code)) {
}

int S3Error::get_http_status_code() {
  return details.get_http_status_code();
}

std::string& S3Error::to_xml() {
  xml_message = "";
  xml_message = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml_message += "<Error>\n"
                  "  <Code>" + code + "</Code>\n"
                  "  <Message>" + details.get_message() + "</Message>\n"
                  "  <Resource>" + resource_key + "</Resource>\n"
                  "  <RequestId>" + request_id + "</RequestId>\n"
                  "</Error>\n";

  return xml_message;
}
