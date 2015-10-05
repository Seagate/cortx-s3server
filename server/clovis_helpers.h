
#pragma once

#ifndef __MERO_FE_S3_SERVER_CLOVIS_HELPERS_H__
#define __MERO_FE_S3_SERVER_CLOVIS_HELPERS_H__

#ifdef __cplusplus
#define EXTERN_C_BLOCK_BEGIN    extern "C" {
#define EXTERN_C_BLOCK_END      }
#define EXTERN_C_FUNC           extern "C"
#else
#define EXTERN_C_BLOCK_BEGIN
#define EXTERN_C_BLOCK_END
#define EXTERN_C_FUNC
#endif


EXTERN_C_BLOCK_BEGIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
/* libevhtp */
#include <evhtp.h>

/* KD */
#include <sys/stat.h>
#include <math.h>

#include "module/instance.h"
#include "mero/init.h"

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

/* For htonl */
#include <arpa/inet.h>

struct s3_metadata_object
{
  size_t metadata_size;
  size_t object_size;
  /* object id */
  int version;
};

int init_clovis(const char *clovis_local_addr, const char *clovis_confd_addr, const char *clovis_prof);
void fini_clovis(void);

//int clovis_read(struct m0_uint128 object_id, int clovis_block_size,
//      int start_block_idx, int clovis_block_count, struct m0_bufvec *data);

//int clovis_write(struct m0_uint128 object_id, int clovis_block_size, int clovis_block_count, int start_block_idx, int data_extents, struct evbuffer_iovec *vec_in, const char *streamed_metadata, int streamed_metadata_len, char **err_msg);

//int clovis_delete(struct m0_uint128 object_id);

EXTERN_C_BLOCK_END

#endif
