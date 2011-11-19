#include "stdlib/sl_linked_list.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct sl_linked_list* list = NULL;
  struct sl_linked_list* list1 = NULL;
  struct sl_node* node0 = NULL;
  struct sl_node* node1 = NULL;
  struct sl_node* node2 = NULL;
  void* data = NULL;
  size_t length = 0;
  int i = 0;
  ALIGN(4) char a = 'a';
  
  CHECK(sl_create_linked_list(1, 3, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_create_linked_list(1, 3, NULL, &list), SL_ALIGNMENT_ERROR);
  CHECK(sl_create_linked_list(1, 4, NULL, &list), SL_NO_ERROR);

  CHECK(sl_linked_list_length(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(NULL, &length), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_length(list, &length), SL_NO_ERROR);
  CHECK(length, 0);

  CHECK(sl_linked_list_add(NULL, NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(NULL, NULL, &node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_add(list, &a, &node0), SL_NO_ERROR);
  CHECK(sl_linked_list_length(list, &length), SL_NO_ERROR);
  CHECK(length, 1);

  CHECK(sl_linked_list_head(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(NULL, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_head(list, &node1), SL_NO_ERROR);
  CHECK(node0, node1);

  CHECK(sl_linked_list_add(list, &a, NULL), SL_NO_ERROR);
  CHECK(sl_linked_list_length(list, &length), SL_NO_ERROR);
  CHECK(length, 2);

  CHECK(sl_linked_list_head(list, &node0), SL_NO_ERROR);
  CHECK(sl_next_node(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(node0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(NULL, &node1), SL_INVALID_ARGUMENT);
  CHECK(sl_next_node(node0, &node1), SL_NO_ERROR);
  NCHECK(node1, NULL);
  NCHECK(node0, node1);

  CHECK(sl_next_node(node1, &node2), SL_NO_ERROR);
  NCHECK(node0, node1);
  NCHECK(node0, node2);
  NCHECK(node1, node2);
  NCHECK(node0, NULL);
  NCHECK(node1, NULL);
  CHECK(node2, NULL);

  CHECK(sl_next_node(node2, &node2), SL_INVALID_ARGUMENT);
  CHECK(node2, NULL);

  CHECK(sl_previous_node(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(node1, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_previous_node(node1, &node2), SL_NO_ERROR);
  CHECK(node2, node0);
  NCHECK(node2, node1);
  NCHECK(node2, NULL);

  CHECK(sl_previous_node(node0, &node2), SL_NO_ERROR);
  CHECK(node2, NULL);

  CHECK(sl_linked_list_remove(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(list, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(NULL, node0), SL_INVALID_ARGUMENT);
  CHECK(sl_linked_list_remove(list, node0), SL_NO_ERROR);
  CHECK(sl_linked_list_length(list, &length), SL_NO_ERROR);
  CHECK(length, 1);
  CHECK(sl_linked_list_head(list, &node2), SL_NO_ERROR);
  CHECK(node2, node1);

  CHECK(sl_linked_list_remove(list, node2), SL_NO_ERROR);
  CHECK(sl_linked_list_length(list, &length), SL_NO_ERROR);
  CHECK(length, 0);
  CHECK(sl_linked_list_head(list, &node2), SL_NO_ERROR);
  CHECK(node2, NULL);

  CHECK(sl_create_linked_list
        (sizeof(int),ALIGNOF(int), &mem_default_allocator, &list1),SL_NO_ERROR);
  CHECK(sl_linked_list_add(list1, (int[]){0}, NULL), SL_NO_ERROR);
  CHECK(sl_linked_list_add(list1, (int[]){2}, NULL), SL_NO_ERROR);
  CHECK(sl_linked_list_add(list1, (int[]){5}, &node0), SL_NO_ERROR);
  CHECK(sl_linked_list_length(list1, &length), SL_NO_ERROR);
  CHECK(length, 3);

  CHECK(sl_node_data(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(node0, NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(NULL, &data), SL_INVALID_ARGUMENT);
  CHECK(sl_node_data(node0, &data), SL_NO_ERROR);
  CHECK(*(int*)data, 5);

  CHECK(sl_linked_list_head(list1, &node0), SL_NO_ERROR);
  NCHECK(node0, NULL);
  CHECK(sl_node_data(node0, &data), SL_NO_ERROR);
  i = *(int*)data;
  CHECK((i == 0 || i == 2 || i == 5), true);
  CHECK(sl_next_node(node0, &node1), SL_NO_ERROR);
  NCHECK(node0, node1);
  CHECK(sl_node_data(node1, &data), SL_NO_ERROR);
  i = *(int*)data;
  CHECK((i == 0 || i == 2 || i == 5), true);
  CHECK(sl_next_node(node1, &node2), SL_NO_ERROR);
  NCHECK(node0, node2);
  NCHECK(node1, node2);
  CHECK(sl_node_data(node2, &data), SL_NO_ERROR);
  i = *(int*)data;
  CHECK((i == 0 || i == 2 || i == 5), true);

  #ifndef NDEBUG
  /* This test is only valid if this utest is linked with the libsl compiled in
   * debug mode. */
  CHECK(sl_linked_list_remove(list, node0), SL_INVALID_ARGUMENT);
  #endif

  CHECK(sl_linked_list_remove(list1, node1), SL_NO_ERROR);
  node1 = NULL;
  CHECK(sl_next_node(node0, &node1), SL_NO_ERROR);
  CHECK(node1, node2);
  CHECK(sl_next_node(node1, &node1), SL_NO_ERROR);
  CHECK(node1, NULL);
  CHECK(sl_previous_node(node2, &node1), SL_NO_ERROR);
  CHECK(node1, node0);
  CHECK(sl_previous_node(node1, &node1), SL_NO_ERROR);
  CHECK(node1, NULL);

  CHECK(sl_clear_linked_list(NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_clear_linked_list(list1), SL_NO_ERROR);
  CHECK(sl_linked_list_length(list, &length), SL_NO_ERROR);
  CHECK(length, 0);
  CHECK(sl_linked_list_head(list, &node2), SL_NO_ERROR);
  CHECK(node2, NULL);

  CHECK(sl_free_linked_list(NULL), SL_INVALID_ARGUMENT);
  CHECK(sl_free_linked_list(list), SL_NO_ERROR);
  CHECK(sl_free_linked_list(list1), SL_NO_ERROR);

  CHECK(MEM_ALLOCATED_SIZE(), 0);

  return 0;
}

