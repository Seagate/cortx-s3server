 */

#pragma once

#ifndef __S3_SERVER_S3_URI_TO_MERO_OID_H__
#define __S3_SERVER_S3_URI_TO_MERO_OID_H__

#include "s3_common.h"
#include "s3_clovis_wrapper.h"

EXTERN_C_BLOCK_BEGIN
#include "clovis/clovis.h"
EXTERN_C_BLOCK_END

int S3UriToMeroOID(std::shared_ptr<ClovisAPI> s3_clovis_api,
                   const char *uri_name, const std::string &request_id,
                   m0_uint128 *ufid,
                   S3ClovisEntityType type = S3ClovisEntityType::object);
#endif
