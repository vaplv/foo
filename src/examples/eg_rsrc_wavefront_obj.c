#include "resources/rsrc_context.h"
#include "resources/rsrc_wavefront_obj.h"
#include "sys/sys.h"
#include <assert.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define RSRC(func) \
  do { \
    if((rsrc_##func) != RSRC_NO_ERROR) { \
      fprintf(stderr, "error:%s:%d\n", __FILE__, __LINE__); \
      exit(-1); \
    } \
  } while(0)

int
main(int argc, char** argv)
{
  struct timeval t0, t1;
  suseconds_t us;
  struct rsrc_context* ctxt = NULL;
  struct rsrc_wavefront_obj* wobj = NULL;
  enum rsrc_error rsrc_err = RSRC_NO_ERROR;
  int err = 0;


  if(argc != 2) {
    printf("usage: %s WAVEFRONT_OBJ\n", argv[0]);
    goto error;
  }

  RSRC(create_context(&ctxt));
  RSRC(create_wavefront_obj(ctxt, &wobj));

  err = gettimeofday(&t0, NULL);
  assert(err == 0);

  rsrc_err = rsrc_load_wavefront_obj(ctxt, wobj, argv[1]);
  if(rsrc_err != RSRC_NO_ERROR) {
    fprintf(stderr, "Error loading the wavefront obj: %s\n", argv[1]);
    goto error;
  }

  err = gettimeofday(&t1, NULL);
  assert(err == 0);

  us = (t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec);
  printf("Elapsed time: %.3f ms\n", us / 1000.f);

exit:
  if(wobj)
    RSRC(free_wavefront_obj(ctxt, wobj));
  if(ctxt)
    RSRC(free_context(ctxt));

  return err;

error:
  err = -1;
  goto exit;
}

