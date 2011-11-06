#ifndef SL_LINKED_LIST_H
#define SL_LINKED_LIST_H

#include "stdlib/sl_error.h"
#include <stddef.h>

struct sl_node;
struct sl_linked_list;

extern enum sl_error
sl_create_linked_list
  (size_t data_size,
   size_t data_alignment,
   struct sl_linked_list** out_list);

extern enum sl_error
sl_free_linked_list
  (struct sl_linked_list* list);

extern enum sl_error
sl_clear_linked_list
  (struct sl_linked_list* list);

extern enum sl_error
sl_linked_list_add
  (struct sl_linked_list* list,
   const void* data,
   struct sl_node** out_node);

extern enum sl_error
sl_linked_list_remove
  (struct sl_linked_list* list,
   struct sl_node* node);

extern enum sl_error
sl_linked_list_length
  (struct sl_linked_list* list,
   size_t* len);

extern enum sl_error
sl_linked_list_head
  (struct sl_linked_list* list,
   struct sl_node** out_node);

extern enum sl_error
sl_next_node
  (const struct sl_node* node,
   struct sl_node** out_node);

extern enum sl_error
sl_previous_node
  (const struct sl_node* node,
   struct sl_node** out_node);

extern enum sl_error
sl_node_data
  (const struct sl_node* node,
   void** out_data);

#endif /* SL_LINKED_LIST_H */

