
#include <unistd.h>

#include "s3_common.h"

#include "s3_clovis_rw_common.h"
#include "s3_clovis_config.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_scope     clovis_uber_scope;
extern struct m0_clovis_container clovis_container;

S3ClovisKVSReader::S3ClovisKVSReader(std::shared_ptr<S3RequestObject> req) : request(req),state(S3ClovisKVSReaderOpState::start) {

}

void S3ClovisKVSReader::get_keyval(std::string index_name, std::string key, std::function<void(void)> on_success, std::function<void(void)> on_failed) {

  int rc = 0;
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  // TODO
}

void S3ClovisKVSReader::get_keyval_successful() {
  state = S3ClovisKVSReaderOpState::present;
  this->handler_on_success();
}

void S3ClovisKVSReader::get_keyval_failed() {
  state = S3ClovisKVSReaderOpState::failed;
  this->handler_on_failed();
}
