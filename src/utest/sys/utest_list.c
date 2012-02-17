#include "sys/list.h"
#include "sys/mem_allocator.h"
#include "sys/sys.h"
#include "utest/utest.h"
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

  init_node(&list);
  init_node(&list1);
  init_node(&elmt0.node);
  init_node(&elmt1.node);
  init_node(&elmt2.node);

  CHECK(is_list_empty(&list), 1);

  elmt0.c = 'a';
  list_add(&list,  &elmt0.node);
  CHECK(is_list_empty(&list), 0);
  CHECK(list_head(&list), &elmt0.node);

  elmt1.c = 'b';
  list_add(&list,  &elmt1.node);
  CHECK(is_list_empty(&list), 0);
  CHECK(elmt1.node.next, &elmt0.node);
  CHECK(elmt1.node.prev, &list);
  CHECK(elmt1.node.next->prev, &elmt1.node);
  CHECK(list_head(&list), &elmt1.node);

  elmt2.c = 'c';
  list_add_tail(&list,  &elmt2.node);
  CHECK(is_list_empty(&list), 0);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt0.node);
  CHECK(elmt2.node.prev->prev, &elmt1.node);
  CHECK(elmt1.node.next->next, &elmt2.node);
  CHECK(elmt0.node.next, &elmt2.node);
  CHECK(list_head(&list), &elmt1.node);
  CHECK(list_tail(&list), &elmt2.node);

  list_del(&elmt0.node);
  CHECK(is_list_empty(&list), 0);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &list);
  CHECK(list_head(&list), &elmt1.node);
  CHECK(list_tail(&list), &elmt2.node);

  list_del(&elmt2.node);
  CHECK(is_list_empty(&list), 0);
  CHECK(elmt1.node.next, &list);
  CHECK(elmt1.node.prev, &list);
  CHECK(list_head(&list), &elmt1.node);
  CHECK(list_tail(&list), &elmt1.node);

  list_del(&elmt1.node);
  CHECK(is_list_empty(&list), 1);

  list_add(&list,  &elmt2.node);
  list_add(&list,  &elmt1.node);
  list_add(&list,  &elmt0.node);
  CHECK(is_list_empty(&list), 0);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &elmt0.node);
  CHECK(elmt0.node.next, &elmt1.node);
  CHECK(elmt0.node.prev, &list);
  CHECK(list_head(&list), &elmt0.node);
  CHECK(list_tail(&list), &elmt2.node);

  CHECK(is_list_empty(&list1), 1);
  list_move(&elmt1.node, &list1);
  CHECK(is_list_empty(&list), 0);
  CHECK(is_list_empty(&list1), 0);
  CHECK(elmt2.node.next, &list);
  CHECK(elmt2.node.prev, &elmt0.node);
  CHECK(elmt1.node.next, &list1);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &elmt2.node);
  CHECK(elmt0.node.prev, &list);
  CHECK(list_head(&list), &elmt0.node);
  CHECK(list_tail(&list), &elmt2.node);
  CHECK(list_head(&list1), &elmt1.node);
  CHECK(list_tail(&list1), &elmt1.node);

  list_move_tail(&elmt2.node, &list1);
  CHECK(is_list_empty(&list), 0);
  CHECK(is_list_empty(&list1), 0);
  CHECK(elmt2.node.next, &list1);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &list);
  CHECK(elmt0.node.prev, &list);
  CHECK(list_head(&list), &elmt0.node);
  CHECK(list_tail(&list), &elmt0.node);
  CHECK(list_head(&list1), &elmt1.node);
  CHECK(list_tail(&list1), &elmt2.node);

  list_move(&elmt0.node, &list1);
  CHECK(is_list_empty(&list), 1);
  CHECK(is_list_empty(&list1), 0);
  CHECK(elmt2.node.next, &list1);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &elmt0.node);
  CHECK(elmt0.node.next, &elmt1.node);
  CHECK(elmt0.node.prev, &list1);
  CHECK(list_head(&list1), &elmt0.node);
  CHECK(list_tail(&list1), &elmt2.node);

  i = 0;
  LIST_FOR_EACH(n, &list1) {
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    CHECK(e->c, 'a' + i);
    ++i;
  }
  CHECK(i, 3);

  i = 3;
  LIST_FOR_EACH_REVERSE(n, &list1) {
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    --i;
    CHECK(e->c, 'a' + i);
  }
  CHECK(i, 0);

  i = 0;
  LIST_FOR_EACH_SAFE(n, tmp, &list1) {
    list_move_tail(n, &list);
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    CHECK(e->c, 'a' + i);
    ++i;
  }
  CHECK(i, 3);
  CHECK(is_list_empty(&list1), 1);
  CHECK(is_list_empty(&list), 0);

  i = 3;
  LIST_FOR_EACH_REVERSE_SAFE(n, tmp, &list) {
    list_move(n, &list1);
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    --i;
    CHECK(e->c, 'a' + i);
  }
  CHECK(i, 0);
  CHECK(is_list_empty(&list1), 0);
  CHECK(is_list_empty(&list), 1);

  i = 0;
  LIST_FOR_EACH(n, &list1) {
    struct elmt* e = CONTAINER_OF(n, struct elmt, node);
    CHECK(e->c, 'a' + i);
    ++i;
  }
  CHECK(i, 3);

  list_move(&elmt1.node, &list1);
  CHECK(elmt2.node.next, &list1);
  CHECK(elmt2.node.prev, &elmt0.node);
  CHECK(elmt1.node.next, &elmt0.node);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &elmt2.node);
  CHECK(elmt0.node.prev, &elmt1.node);
  CHECK(list_head(&list1), &elmt1.node);
  CHECK(list_tail(&list1), &elmt2.node);

  list_move_tail(&elmt0.node, &list1);
  CHECK(elmt2.node.next, &elmt0.node);
  CHECK(elmt2.node.prev, &elmt1.node);
  CHECK(elmt1.node.next, &elmt2.node);
  CHECK(elmt1.node.prev, &list1);
  CHECK(elmt0.node.next, &list1);
  CHECK(elmt0.node.prev, &elmt2.node);
  CHECK(list_head(&list1), &elmt1.node);
  CHECK(list_tail(&list1), &elmt0.node);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;
}

