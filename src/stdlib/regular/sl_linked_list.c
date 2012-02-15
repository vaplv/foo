#include "stdlib/sl.h"
#include "stdlib/sl_linked_list.h"
#include "sys/sys.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static FINLINE void
add_node(struct sl_node* node, struct sl_node* prev, struct sl_node* next)
{
  assert(node && prev && next);
  next->prev = node;
  node->next = next;
  node->prev = prev;
  prev->next = node;
}

static FINLINE void
del_node(struct sl_node* prev, struct sl_node* next)
{
  assert(prev && next);
  next->prev = prev;
  prev->next = next;
}

/******************************************************************************
 *
 * Implementation of the linked list functions.
 *
 ******************************************************************************/
EXPORT_SYM enum sl_error
sl_init_node(struct sl_node* node)
{
  if(!node)
    return SL_INVALID_ARGUMENT;
  node->next = node;
  node->prev = node;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_head(struct sl_node* node, struct sl_node** out_node)
{
  bool is_empty = false;

  if(!node || !out_node)
    return SL_INVALID_ARGUMENT;

  SL(is_list_empty(node, &is_empty));
  if(true == is_empty)
    return SL_INVALID_ARGUMENT;

  *out_node = node->next;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_tail(struct sl_node* node, struct sl_node** out_node)
{
  bool is_empty = false;

  if(!node || !out_node)
    return SL_INVALID_ARGUMENT;

  SL(is_list_empty(node, &is_empty));
  if(true == is_empty)
    return SL_INVALID_ARGUMENT;

  *out_node = node->prev;
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_add(struct sl_node* list, struct sl_node* node)
{
  if(!list || !node || node->next != node)
    return SL_INVALID_ARGUMENT;
  add_node(node, list, list->next);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_add_tail(struct sl_node* list, struct sl_node* node)
{
  if(!list || !node || node->next != node)
    return SL_INVALID_ARGUMENT;
  add_node(node, list->prev, list);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_del(struct sl_node* node)
{
  if(!node)
    return SL_INVALID_ARGUMENT;
  del_node(node->prev, node->next);
  SL(init_node(node));
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_move(struct sl_node* node, struct sl_node* list)
{
  if(!node || !list)
    return SL_INVALID_ARGUMENT;
  del_node(node->prev, node->next);
  add_node(node, list, list->next);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_list_move_tail(struct sl_node* node, struct sl_node* list)
{
  if(!node || !list)
    return SL_INVALID_ARGUMENT;
  del_node(node->prev, node->next);
  add_node(node, list->prev, list);
  return SL_NO_ERROR;
}

EXPORT_SYM enum sl_error
sl_is_list_empty(struct sl_node* node, bool* is_empty)
{
  if(!node || !is_empty)
    return SL_INVALID_ARGUMENT;
  *is_empty = (node->next == node);
  return SL_NO_ERROR;
}

