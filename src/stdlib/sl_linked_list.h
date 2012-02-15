#ifndef SL_LINKED_LIST_H
#define SL_LINKED_LIST_H

#include "stdlib/sl_error.h"
#include <stdbool.h>
#include <stddef.h>

struct sl_node {
  struct sl_node* next;
  struct sl_node* prev;
};

#define SL_LIST_FOR_EACH(pos, list) \
  for(pos = (list)->next; pos != (list); pos = pos->next)

#define SL_LIST_FOR_EACH_REVERSE(pos, list) \
  for(pos = (list)->prev; pos != (list); pos = pos->prev)

/* Safe against removal of list entry. */
#define SL_LIST_FOR_EACH_SAFE(pos, tmp, list) \
  for(pos = (list)->next, tmp = pos->next; \
      pos != (list); \
      pos = tmp, tmp = pos->next)

/* Safe against removal of list entry. */
#define SL_LIST_FOR_EACH_REVERSE_SAFE(pos, tmp, list) \
  for(pos = (list)->prev, tmp = pos->prev; \
      pos != (list); \
      pos = tmp, tmp = pos->prev)

extern enum sl_error
sl_init_node
  (struct sl_node* node);

extern enum sl_error
sl_list_head
  (struct sl_node* node,
   struct sl_node** head);

extern enum sl_error
sl_list_tail
  (struct sl_node* node,
   struct sl_node** tail);

extern enum sl_error
sl_list_add
  (struct sl_node* list,
   struct sl_node* node);

extern enum sl_error
sl_list_add_tail
  (struct sl_node* list,
   struct sl_node* node);

extern enum sl_error
sl_list_del
  (struct sl_node* node);

extern enum sl_error
sl_list_move
  (struct sl_node* node, /* Node to move. */
   struct sl_node* list); /* Destination list. */

extern enum sl_error
sl_list_move_tail
  (struct sl_node* node, /* Node to move. */
   struct sl_node* list); /* Destination list. */

extern enum sl_error
sl_is_list_empty
  (struct sl_node* list,
   bool* is_empty);

#endif /* SL_LINKED_LIST_H */

