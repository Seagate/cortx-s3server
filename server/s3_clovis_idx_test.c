#include <stdio.h>
#include <stdlib.h>
#include "module/instance.h"
#include "mero/init.h"
#include "lib/memory.h"
#include "clovis/clovis.h"
/*
#include "clovis/st/clovis_st.h"
#include "clovis/st/clovis_st_misc.h"
#include "clovis/st/clovis_st_assert.h"
*/

static struct m0_clovis_container clovis_st_idx_container;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) ((sizeof (a)) / (sizeof (a)[0]))
#endif


struct key_val
{
  char *key;
  char *value;
};

struct m0_bufvec *keys;
struct m0_bufvec *vals;


struct key_val *key_value;


static void idx_bufvec_free(struct m0_bufvec *bv)
{
        uint32_t i;

        if (bv != NULL)
                return;

        if (bv->ov_buf != NULL) {
                for (i = 0; i < bv->ov_vec.v_nr; ++i)
                {
                  if(bv->ov_buf[i] != NULL)
                    free(bv->ov_buf[i]);
                }
                free(bv->ov_buf);
        }
        if(bv->ov_vec.v_count != NULL)
          free(bv->ov_vec.v_count);
        free(bv);
}

static struct m0_bufvec* idx_bufvec_alloc(int nr)
{
        struct m0_bufvec *bv;
        char **buffer;

        bv = (m0_bufvec*)m0_alloc(sizeof(*bv));
        if (bv == NULL)
                return NULL;

        bv->ov_vec.v_nr = nr;
        bv->ov_vec.v_count = (m0_bcount_t *)calloc(nr,sizeof(m0_bcount_t));
        /*MEM_ALLOC_ARR(bv->ov_vec.v_count, nr);*/
        if (bv->ov_vec.v_count == NULL)
                goto FAIL;

        /*M0_ALLOC_ARR(bv->ov_buf, nr); */
        buffer =(char **)calloc(nr,sizeof(char *));
        bv->ov_buf = (void **)buffer;
        if (bv->ov_buf == NULL)
          goto FAIL;

        return bv;

FAIL:
        m0_bufvec_free(bv);
        return NULL;
}


int cli_parse_args(int argc,char ** argv)
{
  char    ** largv;
  int        largc;
  int        nKeys;
  int        no_of_keys,got_key;

  largc = argc;
  largv = argv;

  if(argc == 1)
    return -1;


  nKeys = (argc -1)/2 + 0.5;

  /*key_value = (struct key_val *)calloc(argc-1, (struct key_val)); */

  keys = idx_bufvec_alloc(nKeys);
  vals = idx_bufvec_alloc(nKeys);

  no_of_keys = got_key = 0;
  while (--largc && *++largv != NULL)
  {

    if(got_key)
    {
      vals->ov_vec.v_count[no_of_keys] = strlen(*largv);
      vals->ov_buf[no_of_keys++] = *largv;
      /*key_value[no_of_keys].value = calloc(1,(sizeof(char)*strlen(*largv) + 1));
      strcpy(key_value[no_of_keys++].value,*largv); 
      */
      
      got_key = 0;
    }
    else
    {
      /*key_value[no_of_keys].key = calloc(1,(sizeof(char)*strlen(*largv) + 1));
      strcpy(key_value[no_of_keys].key,*largv);
      */
      keys->ov_vec.v_count[no_of_keys] = strlen(*largv);
      keys->ov_buf[no_of_keys] = *largv;
      got_key = 1; 
    }


  } 
  if(got_key == 1)
  {
    printf("Key without any value");
    /*free(key_value); */
    if (keys) idx_bufvec_free(keys);
    if (vals) idx_bufvec_free(vals);

    return -1;
  }
  return 0;
}
   
   
int InsertToIndex(struct m0_uint128 id)
{
   int                     rc;
   struct m0_clovis_op    *ops[1] = {NULL};
   struct m0_clovis_idx    idx;

   memset(&idx, 0, sizeof(idx));
   ops[0] = NULL;


   m0_clovis_idx_init(&idx,&clovis_st_idx_container.co_scope, &id);
   /*clovis_st_mark_entity(&idx->in_entity); */


   /*m0_clovis_entity_create(&idx.ob_entity, &ops[0]); */


   rc = m0_clovis_idx_op(&idx,M0_CLOVIS_IC_PUT, keys, vals, &ops[0]);
   if(rc  != 0)
   {
     return rc;
   }

   m0_clovis_op_launch(ops, ARRAY_SIZE(ops)); 


   rc = m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		         M0_TIME_NEVER);
   if (rc < 0) return rc;
	
   m0_clovis_op_fini(ops[0]);
   m0_clovis_op_free(ops[0]);
   m0_clovis_entity_fini(&idx.in_entity);

   return 0;

}



int main(int argc, char ** argv)
{
        int rc;
        struct m0_uint128 id;
	if(argc == 1)
  	  return 0;

	rc = cli_parse_args(argc,argv);
        if(rc != 0)
          return rc;

 	/* Will be using get_oid_using_hash */
 	/*clovis_oid_get(&id);*/
        id = M0_CLOVIS_ID_APP;
        id.u_lo = 7778; 
 
 	rc = InsertToIndex(id);
 	if(rc != 0)
 	{
   	  printf("CreateIndex failed\n");
 	}


}
