#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>

#include "clovis_helpers.h"

#include <openssl/md5.h>

extern struct m0_clovis_scope     clovis_uber_scope;

static int create_object(struct m0_uint128 id)
{
  int                  rc;
  struct m0_clovis_obj obj;
  struct m0_clovis_op *ops[1] = {NULL};

  memset(&obj, 0, sizeof(struct m0_clovis_obj));

  m0_clovis_obj_init(&obj, &clovis_uber_scope, &id);

  m0_clovis_entity_create(&obj.ob_entity, &ops[0]);

  m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

  rc = m0_clovis_op_wait(
    ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
    m0_time_from_now(3,0));

  m0_clovis_op_fini(ops[0]);
  m0_clovis_op_free(ops[0]);
  m0_clovis_entity_fini(&obj.ob_entity);

  return rc;
}

void buffer_to_hex(unsigned char *buf, int len, unsigned char *str)
{
  int i = 0;
  for (i = 0; i < len; i++)
  {
    sprintf((char *)(&str[i*2]), "%02x", (unsigned int)buf[i]);
  }
}

// static void map_to_clovis_vec(struct evbuffer_iovec *vec_in, struct m0_bufvec *vec_out, int clovis_block_count, struct m0_indexvec *ext)
// {
//   int i;
//    /*convert evbuffer_iovec to m0_bufvec*/
//     for( i = 0; i < clovis_block_count; i++) {
//       data_vec.ov_buf[i] = vec_in[i].iov_base;
//       (*ext).iv_vec.v_count[i] = vec_in[i].iov_len;
//     }
// }

static int write_data_to_object(struct m0_uint128 id,
        struct m0_indexvec *ext,
        struct m0_bufvec *data,
        struct m0_bufvec *attr)
{
  int                  rc;
  struct m0_clovis_obj obj;
  struct m0_clovis_op *ops[1] = {NULL};

  memset(&obj, 0, sizeof(struct m0_clovis_obj));

  /* Set the object entity we want to write */
  m0_clovis_obj_init(&obj, &clovis_uber_scope, &id);

  /* Create the write request */
  m0_clovis_obj_op(&obj, M0_CLOVIS_OC_WRITE,
       ext, data, attr, 0, &ops[0]);

  /* Launch the write request*/
  m0_clovis_op_launch(ops, 1);

  /* wait */
  rc = m0_clovis_op_wait(ops[0],
      M0_BITS(M0_CLOVIS_OS_FAILED,
        M0_CLOVIS_OS_STABLE),
      M0_TIME_NEVER);

  /* fini and release */
  m0_clovis_op_fini(ops[0]);
  m0_clovis_op_free(ops[0]);
  m0_clovis_entity_fini(&obj.ob_entity);

  return rc;
}

int clovis_write(struct m0_uint128 object_id, int clovis_block_size, int clovis_block_count,
      int start_block_idx, int data_extents, struct evbuffer_iovec *vec_in, const char *streamed_metadata, int streamed_metadata_len, char **err_msg)
{
  int                i;
  int                rc;
  uint64_t           last_index;
  struct m0_indexvec ext;
  struct m0_bufvec   data;
  struct m0_bufvec   attr;
  int current_block_idx = 0;
  clovis_block_count = clovis_block_count + 1 ;/*+1 metadata*/

  /*
   * Read data from source file.
   * Allocate <clovis_block_count> * 4K buffers for data first.
   */
  rc = m0_bufvec_alloc(&data, clovis_block_count, clovis_block_size);
  if (rc != 0)
    return rc;

  // map_to_clovis_vec(vec_in, &data, clovis_block_count);

  /*
   * Prepare indexvec for write: <clovis_block_count> from the
   * beginning of the object.
   */
  rc = m0_bufvec_alloc(&attr, clovis_block_count, 1);
  if(rc != 0)
    return rc;

  rc = m0_indexvec_alloc(&ext, clovis_block_count);
  if (rc != 0)
    return rc;

  last_index = clovis_block_size * start_block_idx;
  /* Write the metadata */
  /* [streamlength, stream] */
  uint32_t nbo_stream_len = htonl(streamed_metadata_len);
  // printf("Writing @ %p bytes = %ld\n", data.ov_buf[i], sizeof(nbo_stream_len));
  memcpy(data.ov_buf[current_block_idx], &nbo_stream_len, sizeof(nbo_stream_len));

  // printf("Writing @ %p bytes = %d\n", data.ov_buf[i] + sizeof(nbo_stream_len), streamed_metadata_len);
  memcpy(data.ov_buf[current_block_idx] + sizeof(nbo_stream_len), streamed_metadata, streamed_metadata_len);

  /* Now start writing the data */
  int data_to_consume = clovis_block_size;
  int pending_from_current_extent = 0;
  int idx_within_block = 0;
  int idx_within_extent = 0;
  current_block_idx++;
  for (i = 0; i < data_extents; i++) {
    // printf("processing extent = %d of total %d\n", i, data_extents);
    pending_from_current_extent = vec_in[i].iov_len;
    /* KD xxx - we need to avoid this copy */
    while (1) /* consume all data in extent */
    {
      if (pending_from_current_extent == data_to_consume)
      {
        /* consume all */
        // printf("Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", pending_from_current_extent, i, current_block_idx, idx_within_block);
        // printf("memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, vec_in[i].iov_base + idx_within_extent, pending_from_current_extent);
        memcpy(data.ov_buf[current_block_idx] + idx_within_block, vec_in[i].iov_base + idx_within_extent, pending_from_current_extent);
        idx_within_block = idx_within_extent = 0;
        data_to_consume = clovis_block_size;
        current_block_idx++;
        break; /* while(1) */
      } else if (pending_from_current_extent < data_to_consume)
      {
        // printf("Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", pending_from_current_extent, i, current_block_idx, idx_within_block);
        // printf("memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, vec_in[i].iov_base + idx_within_extent, pending_from_current_extent);
        memcpy(data.ov_buf[current_block_idx] + idx_within_block, vec_in[i].iov_base + idx_within_extent, pending_from_current_extent);
        idx_within_block = idx_within_block + pending_from_current_extent;
        idx_within_extent = 0;
        data_to_consume = data_to_consume - pending_from_current_extent;
        break; /* while(1) */
      } else /* if (pending_from_current_extent > data_to_consume) */
      {
        // printf("Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", data_to_consume, i, current_block_idx, idx_within_block);
        // printf("memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, vec_in[i].iov_base + idx_within_extent, data_to_consume);
        memcpy(data.ov_buf[current_block_idx] + idx_within_block, vec_in[i].iov_base + idx_within_extent, data_to_consume);
        idx_within_block = 0;
        idx_within_extent = idx_within_extent + data_to_consume;
        pending_from_current_extent = pending_from_current_extent - data_to_consume;
        data_to_consume = clovis_block_size;
        current_block_idx++;
      }
    }
  }

  for(i = 0; i < clovis_block_count; i++)
  {
    ext.iv_index[i] = last_index ;
    ext.iv_vec.v_count[i] = /*vec_in[i].iov_len*/clovis_block_size;
    last_index += /*vec_in[i].iov_len*/clovis_block_size;

    /* we don't want any attributes */
    attr.ov_vec.v_count[i] = 0;
  }
  /* Create the object */
  rc = create_object(object_id);
  if (rc != 0) {
    *err_msg = "Can't create object!\n";
    return rc;
  }

  /* Copy data to the object*/
  rc = write_data_to_object(object_id, &ext, &data, &attr);
  if (rc != 0) {
    *err_msg = "Writing to object failed!\n";
    return rc;
  }

  /* Free bufvec's and indexvec's */
  m0_indexvec_free(&ext);
  m0_bufvec_free(&data);
  m0_bufvec_free(&attr);

  return 0;
}

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
