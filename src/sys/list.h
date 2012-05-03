#ifndef LIST_H
#define LIST_H

#include "sys/sys.h"
#include <assert.h>

struct list_node {
  struct list_node* next;
  struct list_node* prev;
};

/******************************************************************************
 *
 * Private functions.
 *
 ******************************************************************************/
static FINLINE void
add_node__(struct list_node* node, struct list_node* prev, struct list_node* next)
{
  assert(node && prev && next);
  next->prev = node;
  node->next = next;
  node->prev = prev;
  prev->next = node; 
}

static FINLINE void
del_node__(struct list_node* prev, struct list_node* next)
{
  assert(prev && next);
  next->prev = prev;
  prev->next = next;
}

/******************************************************************************
 *
 * Helper macros.
 *
 ******************************************************************************/
#define LIST_FOR_EACH(pos, list) \
  for(pos = (list)->next; pos != (list); pos = pos->next)

#define LIST_FOR_EACH_REVERSE(pos, list) \
  for(pos = (list)->prev; pos != (list); pos = pos->prev)

/* Safe against removal of list entry. */
#define LIST_FOR_EACH_SAFE(pos, tmp, list) \
  for(pos = (list)->next, tmp = pos->next; \
      pos != (list); \
      pos = tmp, tmp = pos->next)

/* Safe against removal of list entry. */
#define LIST_FOR_EACH_REVERSE_SAFE(pos, tmp, list) \
  for(pos = (list)->prev, tmp = pos->prev; \
      pos != (list); \
      pos = tmp, tmp = pos->prev)

/******************************************************************************
 *
 * Node list functions.
 *
 ******************************************************************************/
static FINLINE void
list_init(struct list_node* node)
{
  assert(node);
  node->next = node;
  node->prev = node;
}

static FINLINE int
is_list_empty(const struct list_node* node)
{
  assert(node);
  return node->next == node;
}

static FINLINE struct list_node*
list_head(struct list_node* node)
{
  assert(node && !is_list_empty(node));
  return node->next;
}

static FINLINE struct list_node*
list_tail(struct list_node* node)
{
  assert(node && !is_list_empty(node));
  return node->prev;
}

static FINLINE void
list_add(struct list_node* list, struct list_node* node)
{
  assert(list && node && is_list_empty(node));
  add_node__(node, list, list->next);
}

static FINLINE void
list_add_tail(struct list_node* list, struct list_node* node)
{
  assert(list && node && is_list_empty(node));
  add_node__(node, list->prev, list);
}

static FINLINE void
list_del(struct list_node* node)
{
  assert(node);
  del_node__(node->prev, node->next);
  list_init(node);
}

static FINLINE void
list_move(struct list_node* node, struct list_node* list)
{
  assert(node && list);
  del_node__(node->prev, node->next);
  add_node__(node, list, list->next);
}

static FINLINE void
list_move_tail(struct list_node* node, struct list_node* list)
{
  assert(node && list);
  del_node__(node->prev, node->next);
  add_node__(node, list->prev, list);
}

#endif /* LIST_H */

