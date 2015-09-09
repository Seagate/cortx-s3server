
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_URI_TO_MERO_OID_H__
#define __MERO_FE_S3_SERVER_S3_URI_TO_MERO_OID_H__

#include "s3_common.h"

EXTERN_C_BLOCK_BEGIN
#include "module/instance.h"
#include "mero/init.h"

#include "clovis/clovis.h"
EXTERN_C_BLOCK_END

void S3UriToMeroOID(const char* name, struct m0_uint128 *object_id);

#endif
