#include "stdlib/sl_linked_list.h"
#include "stdlib/sl.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

struct sl_node {
  struct sl_node* prev;
  struct sl_node* next;
  void* data;
};

struct sl_linked_list {
  size_t data_size;
  size_t data_alignment;
  struct sl_node* head;
};

/******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static void
free_node(struct sl_node* node) 
{
  assert(node != NULL);
  if(node->data)
    free(node->data);
  free(node);
}

static enum sl_error
alloc_node(size_t data_size, size_t data_alignment, struct sl_node** out_node)
{
  struct sl_node* node = NULL;
  enum sl_error err = SL_NO_ERROR;

  assert(out_node);

  node = calloc(1, sizeof(struct sl_node));
  if(!node) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  node->data = memalign(data_alignment, data_size);
  if(!node->data) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

exit:
  *out_node = node;
  return err;

error:
  if(node) {
    free_node(node);
    node = NULL;
  }
  goto exit;
}

static void
free_all_nodes(struct sl_linked_list* list)
{
  struct sl_node* node = NULL;

  assert(list != NULL);

  node = list->head;
  while(node != NULL) {
    struct sl_node* next_node = node->next;
    free_node(node);
    node = next_node;
  }
}

/******************************************************************************
 *
 * Implementation of the linked list functions.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_create_linked_list
  (struct sl_context* ctxt,
   size_t data_size,
   size_t data_alignment,
   struct sl_linked_list** out_list)
{
  struct sl_linked_list* list = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !out_list) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  
  }

  if(!SL_IS_POWER_OF_2(data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  list = calloc(1, sizeof(struct sl_linked_list));
  if(list == NULL) {
    err = SL_MEMORY_ERROR;
    goto error;
  }

  list->data_size = data_size;
  list->data_alignment = data_alignment;

exit:
  if(out_list)
    *out_list = list;
  return err;

error:
  if(list) {
    free(list);
    list = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_free_linked_list
  (struct sl_context* ctxt,
   struct sl_linked_list* list)
{
  if(!ctxt || !list)
    return SL_INVALID_ARGUMENT;

  free_all_nodes(list);
  free(list);

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_clear_linked_list
  (struct sl_context* ctxt,
   struct sl_linked_list* list)
{
  if(!ctxt || !list)
    return SL_INVALID_ARGUMENT;

  free_all_nodes(list);
  list->head = NULL;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_linked_list_add
  (struct sl_context* ctxt,
   struct sl_linked_list* list,
   const void* data,
   struct sl_node** out_node)
{
  struct sl_node* node = NULL;
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !list || (!data && list->data_size > 0)) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  if(!IS_ALIGNED(data, list->data_alignment)) {
    err = SL_ALIGNMENT_ERROR;
    goto error;
  }

  err = alloc_node(list->data_size, list->data_alignment, &node);
  if(err != SL_NO_ERROR)
    goto error;

  memcpy(node->data, data, list->data_size);

  if(list->head != NULL)
    list->head->prev = node;
  node->next = list->head;
  list->head = node;

exit:
  if(out_node)
    *out_node = node;
  return err;

error:
  if(node) {
    free_node(node);
    node = NULL;
  }
  goto exit;
}

EXPORT_SYM enum sl_error
sl_linked_list_remove
  (struct sl_context* ctxt,
   struct sl_linked_list* list,
   struct sl_node* node)
{
  enum sl_error err = SL_NO_ERROR;

  if(!ctxt || !list || !node) {
    err = SL_INVALID_ARGUMENT;
    goto error;
  }

  #ifndef NDEBUG
  {
    struct sl_node* tmp_node = node;

    while(tmp_node->prev != NULL)
      tmp_node = tmp_node->prev;

    if(tmp_node != list->head) {
      err = SL_INVALID_ARGUMENT;
      goto error;
    }
  }
  #endif

  if(node->next)
    node->next->prev = node->prev;

  if(node->prev)
    node->prev->next = node->next;
  else
    list->head = node->next;

  free_node(node);

exit:
  return err;

error:
  goto exit;
}

EXPORT_SYM enum sl_error
sl_linked_list_length
  (struct sl_context* ctxt,
   struct sl_linked_list* list,
   size_t* len)
{
  struct sl_node* node = NULL;
  size_t i = 0;
  enum sl_error sl_err = SL_NO_ERROR;

  if(!ctxt || !list || !len) {
    sl_err = SL_INVALID_ARGUMENT;
    goto error;
  }

  for(i = 0, node = list->head; node != NULL; ++i, node = node->next);

exit:
  if(len)
    *len = i;
  return sl_err;

error:
  i = 0;
  goto exit;
}

EXPORT_SYM enum sl_error
sl_linked_list_head
  (struct sl_context* ctxt,
   struct sl_linked_list* list,
   struct sl_node** out_node)
{
  if(!ctxt || !list || !out_node)
    return SL_INVALID_ARGUMENT;

  *out_node = list->head;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_next_node
  (struct sl_context* ctxt,
   const struct sl_node* node,
   struct sl_node** out_node)
{
  if(!ctxt || !node || !out_node)
    return SL_INVALID_ARGUMENT;

  *out_node = node->next;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_previous_node
  (struct sl_context* ctxt,
   const struct sl_node* node,
   struct sl_node** out_node)
{
  if(!ctxt || !node || !out_node)
    return SL_INVALID_ARGUMENT;

  *out_node = node->prev;

  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_node_data
  (struct sl_context* ctxt,
   const struct sl_node* node,
   void** out_data)
{
  if(!ctxt || !node || !out_data)
    return SL_INVALID_ARGUMENT;

  *out_data = node->data;

  return SL_NO_ERROR;
}

