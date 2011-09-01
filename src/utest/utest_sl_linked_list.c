#include "stdlib/sl_linked_list.h"
#include "stdlib/sl_context.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct sl_context* ctxt = NULL;
  struct sl_linked_list* list = NULL;
  struct sl_linked_list* list1 = NULL;
  struct sl_node* node0 = NULL;
  struct sl_node* node1 = NULL;
  struct sl_node* node2 = NULL;
  void* data = NULL;
  size_t length = 0;
  int i = 0;

  CHECK(sl_create_context(NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_create_context(&ctxt), SL_NO_ERROR);

  CHECK(sl_create_linked_list(NULL, 0, 0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_create_linked_list(ctxt, 0, 0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_create_linked_list(NULL, 0, 0, &list), SL_INVALID_ARGUMENT);
  CHECK(sl_create_linked_list(ctxt, 0, 3, &list), SL_ALIGNMENT_ERROR);
  CHECK(sl_create_linked_list(ctxt, 0, 4, &list), SL_NO_ERROR);

  CHECK(sl_linked_list_length(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(NULL, list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(ctxt, list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(NULL, NULL, &length), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(ctxt, NULL, &length), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(NULL, list, &length), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(ctxt, list, &length), SL_NO_ERROR);
  CHECK(length, 0);

  CHECK(sl_linked_list_add(NULL, NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(ctxt, NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(NULL, list, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(NULL, NULL, NULL, &node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(ctxt, NULL, NULL, &node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(NULL, list, NULL, &node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(ctxt, list, NULL, &node0), SL_NO_ERROR);
  CHECK(sl_linked_list_length(ctxt, list, &length), SL_NO_ERROR);
  CHECK(length, 1);

  CHECK(sl_linked_list_head(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(NULL, list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(ctxt, list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(NULL, NULL, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(ctxt, NULL, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(NULL, list, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(ctxt, list, &node1), SL_NO_ERROR);
  CHECK(node0, node1);

  CHECK(sl_linked_list_add(ctxt, list, NULL, NULL), SL_NO_ERROR);
  CHECK(sl_linked_list_length(ctxt, list, &length), SL_NO_ERROR);
  CHECK(length, 2);

  CHECK(sl_linked_list_head(ctxt, list, &node0), SL_NO_ERROR);
  CHECK(sl_next_node(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(NULL, node0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(ctxt, node0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(NULL, NULL, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(ctxt, NULL, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(NULL, node0, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(ctxt, node0, &node1), SL_NO_ERROR);
  NCHECK(node1, NULL);
  NCHECK(node0, node1);

  CHECK(sl_next_node(ctxt, node1, &node2), SL_NO_ERROR);
  NCHECK(node0, node1);
  NCHECK(node0, node2);
  NCHECK(node1, node2);
  NCHECK(node0, NULL);
  NCHECK(node1, NULL);
  CHECK(node2, NULL);

  CHECK(sl_next_node(ctxt, node2, &node2), SL_INVALID_ARGUMENT);
  CHECK(node2, NULL);

  CHECK(sl_previous_node(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(NULL, node1, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(ctxt, node1, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(NULL, node1, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(ctxt, node1, &node2), SL_NO_ERROR);
  CHECK(node2, node0);
  NCHECK(node2, node1);
  NCHECK(node2, NULL);

  CHECK(sl_previous_node(ctxt, node0, &node2), SL_NO_ERROR);
  CHECK(node2, NULL);

  CHECK(sl_linked_list_remove(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(NULL, list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(ctxt, list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(NULL, NULL, node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(ctxt, NULL, node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(NULL, list, node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(ctxt, list, node0), SL_NO_ERROR);
  CHECK(sl_linked_list_length(ctxt, list, &length), SL_NO_ERROR);
  CHECK(length, 1);
  CHECK(sl_linked_list_head(ctxt, list, &node2), SL_NO_ERROR);
  CHECK(node2, node1);

  CHECK(sl_linked_list_remove(ctxt, list, node2), SL_NO_ERROR);
  CHECK(sl_linked_list_length(ctxt, list, &length), SL_NO_ERROR);
  CHECK(length, 0);
  CHECK(sl_linked_list_head(ctxt, list, &node2), SL_NO_ERROR);
  CHECK(node2, NULL);

  CHECK(sl_create_linked_list(ctxt, sizeof(int), ALIGNOF(int), &list1),
        SL_NO_ERROR);
  CHECK(sl_linked_list_add(ctxt, list1, (int[]){0}, NULL), SL_NO_ERROR);
  CHECK(sl_linked_list_add(ctxt, list1, (int[]){2}, NULL), SL_NO_ERROR);
  CHECK(sl_linked_list_add(ctxt, list1, (int[]){5}, &node0), SL_NO_ERROR);
  CHECK(sl_linked_list_length(ctxt, list1, &length), SL_NO_ERROR);
  CHECK(length, 3);

  CHECK(sl_node_data(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(ctxt, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(NULL, node0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(ctxt, node0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(NULL, NULL, &data), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(ctxt, NULL, &data), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(NULL, node0, &data), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(ctxt, node0, &data), SL_NO_ERROR);
  CHECK(*(int*)data, 5);

  CHECK(sl_linked_list_head(ctxt, list1, &node0), SL_NO_ERROR);
  NCHECK(node0, NULL);
  CHECK(sl_node_data(ctxt, node0, &data), SL_NO_ERROR);
  i = *(int*)data;
  CHECK((i == 0 || i == 2 || i == 5), true);
  CHECK(sl_next_node(ctxt, node0, &node1), SL_NO_ERROR);
  NCHECK(node0, node1);
  CHECK(sl_node_data(ctxt, node1, &data), SL_NO_ERROR);
  i = *(int*)data;
  CHECK((i == 0 || i == 2 || i == 5), true);
  CHECK(sl_next_node(ctxt, node1, &node2), SL_NO_ERROR);
  NCHECK(node0, node2);
  NCHECK(node1, node2);
  CHECK(sl_node_data(ctxt, node2, &data), SL_NO_ERROR);
  i = *(int*)data;
  CHECK((i == 0 || i == 2 || i == 5), true);

  #ifndef NDEBUG
  /* This test is only valid if this utest is linked with the libsl compiled in
   * debug mode. */
  CHECK(sl_linked_list_remove(ctxt, list, node0), SL_INVALID_ARGUMENT);
  #endif

  CHECK(sl_linked_list_remove(ctxt, list1, node1), SL_NO_ERROR);
  node1 = NULL;
  CHECK(sl_next_node(ctxt, node0, &node1), SL_NO_ERROR);
  CHECK(node1, node2);
  CHECK(sl_next_node(ctxt, node1, &node1), SL_NO_ERROR);
  CHECK(node1, NULL);
  CHECK(sl_previous_node(ctxt, node2, &node1), SL_NO_ERROR);
  CHECK(node1, node0);
  CHECK(sl_previous_node(ctxt, node1, &node1), SL_NO_ERROR);
  CHECK(node1, NULL);

  CHECK(sl_clear_linked_list(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_clear_linked_list(ctxt, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_clear_linked_list(NULL, list1), SL_INVALID_ARGUMENT);
  CHECK(sl_clear_linked_list(ctxt, list1), SL_NO_ERROR);
  CHECK(sl_linked_list_length(ctxt, list, &length), SL_NO_ERROR);
  CHECK(length, 0);
  CHECK(sl_linked_list_head(ctxt, list, &node2), SL_NO_ERROR);
  CHECK(node2, NULL);

  CHECK(sl_free_linked_list(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_free_linked_list(NULL, list), SL_INVALID_ARGUMENT);
  CHECK(sl_free_linked_list(ctxt, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_free_linked_list(ctxt, list), SL_NO_ERROR);
  CHECK(sl_free_linked_list(ctxt, list1), SL_NO_ERROR);

  CHECK(sl_free_context(NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_free_context(ctxt), SL_NO_ERROR);

  return 0;
}

