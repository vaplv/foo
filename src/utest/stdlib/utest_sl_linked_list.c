#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
#include <stdbool.h>
#include <stdlib.h>

int
main(int argc UNUSED, char** argv UNUSED)
{
  struct elmt {
    struct list_node node;
    char c;
  } elmt0, elmt1, elmt2;
  struct list_node list, list1;
  struct list_node* n = NULL;
  struct list_node* tmp = NULL;
  int i = 0;
  bool b = false;

  CHECK(init_node(NULL), SL_INVALID_ARGUMENT);
  CHECK(init_node(&list), SL_NO_ERROR);
  CHECK(init_node(&list1), SL_NO_ERROR);
  CHECK(init_node(&elmt0.node), SL_NO_ERROR);
  CHECK(init_node(&elmt1.node), SL_NO_ERROR);
  CHECK(init_node(&elmt2.node), SL_NO_ERROR);

  CHECK(is_list_empty(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(is_list_empty(&list, NULL), SL_INVALID_ARGUMENT);
  CHECK(is_list_empty(NULL, &b), SL_INVALID_ARGUMENT);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, true);

  elmt0.c = 'a';
  CHECK(list_add(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(list_add(NULL, &elmt0.node), SL_INVALID_ARGUMENT);
  CHECK(list_add(&list,  &elmt0.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(list.next, &elmt0.node);

  elmt1.c = 'b';
  CHECK(list_add(&list,  &elmt1.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt1.node.next, &elmt0.node);
  CHECK(elmt1.node.prev, &list);
  CHECK(elmt1.node.next->prev, &elmt1.node);
  CHECK(list.next, &elmt1.node);

  elmt2.c = 'c';
  CHECK(list_add_tail(&list,  &elmt2.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt0.node);
  CHECK(elmt2.node.prev->prev, &elmt1.node);
  CHECK(elmt1.node.next->next, &elmt2.node);
  CHECK(elmt0.node.next, &elmt2.node);
  CHECK(list.next, &elmt1.node);
  CHECK(list.prev, &elmt2.node);

  CHECK(list_del(NULL), SL_INVALID_ARGUMENT);
  CHECK(list_del(&elmt0.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &list);
  CHECK(list.next, &elmt1.node);
  CHECK(list.prev, &elmt2.node);

  CHECK(list_del(&elmt2.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt1.node.next, &list);
  CHECK(elmt1.node.prev, &list);
  CHECK(list.next, &elmt1.node);
  CHECK(list.prev, &elmt1.node);

  CHECK(list_del(&elmt1.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, true);

  CHECK(list_add(&list,  &elmt2.node), SL_NO_ERROR);
  CHECK(list_add(&list,  &elmt2.node), SL_INVALID_ARGUMENT);
  CHECK(list_add(&list,  &elmt1.node), SL_NO_ERROR);
  CHECK(list_add(&list,  &elmt0.node), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &elmt0.node);
  CHECK(elmt0.node.next, &elmt1.node);
  CHECK(elmt0.node.prev, &list);
  CHECK(list.next, &elmt0.node);
  CHECK(list.prev, &elmt2.node);

  CHECK(is_list_empty(&list1, &b), SL_NO_ERROR);
  CHECK(b, true);
  CHECK(list_move(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(list_move(&elmt1.node, NULL), SL_INVALID_ARGUMENT);
  CHECK(list_move(NULL, &list1), SL_INVALID_ARGUMENT);
  CHECK(list_move(&elmt1.node, &list1), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(is_list_empty(&list1, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt0.node);
  CHECK(elmt1.node.next, &list1);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &elmt2.node);
  CHECK(elmt0.node.prev, &list);
  CHECK(list.next, &elmt0.node);
  CHECK(list.prev, &elmt2.node);
  CHECK(list1.next, &elmt1.node);
  CHECK(list1.prev, &elmt1.node);

  CHECK(list_move_tail(NULL, NULL), SL_INVALID_ARGUMENT);
  CHECK(list_move_tail(&elmt2.node, NULL), SL_INVALID_ARGUMENT);
  CHECK(list_move_tail(NULL, &list1), SL_INVALID_ARGUMENT);
  CHECK(list_move_tail(&elmt2.node, &list1), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(is_list_empty(&list1, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt2.node.next, &list1);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &list);
  CHECK(elmt0.node.prev, &list);
  CHECK(list.next,&elmt0.node);
  CHECK(list.prev, &elmt0.node);
  CHECK(list1.next, &elmt1.node);
  CHECK(list1.prev, &elmt2.node);

  CHECK(list_move(&elmt0.node, &list1), SL_NO_ERROR);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, true);
  CHECK(is_list_empty(&list1, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(elmt2.node.next, &list1);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &elmt0.node);
  CHECK(elmt0.node.next, &elmt1.node);
  CHECK(elmt0.node.prev, &list1);
  CHECK(list1.next, &elmt0.node);
  CHECK(list1.prev, &elmt2.node);

  i = 0;
  SL_LIST_FOR_EACH(n, &list1) {
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    CHECK(e->c, 'a' + i);
    ++i;
  }
  CHECK(i, 3);

  i = 3;
  SL_LIST_FOR_EACH_REVERSE(n, &list1) {
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    --i;
    CHECK(e->c, 'a' + i);
  }
  CHECK(i, 0);

  i = 0;
  SL_LIST_FOR_EACH_SAFE(n, tmp, &list1) {
    CHECK(list_move_tail(n, &list), SL_NO_ERROR);
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    CHECK(e->c, 'a' + i);
    ++i;
  }
  CHECK(i, 3);
  CHECK(is_list_empty(&list1, &b), SL_NO_ERROR);
  CHECK(b, true);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, false);

  i = 3;
  SL_LIST_FOR_EACH_REVERSE_SAFE(n, tmp, &list) {
    CHECK(list_move(n, &list1), SL_NO_ERROR);
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    --i;
    CHECK(e->c, 'a' + i);
  }
  CHECK(i, 0);
  CHECK(is_list_empty(&list1, &b), SL_NO_ERROR);
  CHECK(b, false);
  CHECK(is_list_empty(&list, &b), SL_NO_ERROR);
  CHECK(b, true);

  i = 0;
  SL_LIST_FOR_EACH(n, &list1) {
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    CHECK(e->c, 'a' + i);
    ++i;
  }
  CHECK(i, 3);

  CHECK(list_move(&elmt1.node, &list1), SL_NO_ERROR);
  CHECK(elmt2.node.next, &list1);
  CHECK(elmt2.node.prev, &elmt0.node);
  CHECK(elmt1.node.next, &elmt0.node);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &elmt2.node);
  CHECK(elmt0.node.prev, &elmt1.node);
  CHECK(list1.next, &elmt1.node);
  CHECK(list1.prev, &elmt2.node);

  CHECK(list_move_tail(&elmt0.node, &list1), SL_NO_ERROR);
  CHECK(elmt2.node.next, &elmt0.node);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &list1);
  CHECK(elmt0.node.prev, &elmt2.node);
  CHECK(list1.next, &elmt1.node);
  CHECK(list1.prev, &elmt0.node);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

