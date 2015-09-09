
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_URI_TO_MERO_OID_H__
#define __MERO_FE_S3_SERVER_S3_URI_TO_MERO_OID_H__

#include "module/instance.h"
#include "mero/init.h"

#include "clovis/clovis.h"

void S3UriToMeroOID(const char* name, struct m0_uint128 *object_id);

#endif
