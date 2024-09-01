#include "list.h"
#include "string"
#include <iostream>

struct student {
  int id = 0;
  std::string name;
  struct list_head node;
};

int main() {
{
  student s1{0, "Tony", {}};
  list_inithead(&s1.node);

  for (int i = 0; i < 10; i++) {
    student *s = new student{i, "Tony" + std::to_string(i), {}};
    list_addtail(&s->node, &s1.node);
  }

  /*
  // 遍历
  list_for_each_entry(student, pos, &s1.node, node) {
      std::cout << pos->id << " " << pos->name << std::endl;
  }
  */

  /*
  // 这段在cpp里通不过编译，因为list_container_of返回一个void*
  // cpp不允许void* 和 type*之间隐式转换
  student* pos;
  LIST_FOR_EACH_ENTRY(pos, &s1.node, node)
  {
    student *st = (student *)pos;
    std::cout << st->id << " " << st->name << std::endl;
  }
   */

  // 遍历删除
  /*
  list_for_each_entry_safe(student, pos, &s1.node, node) {
      std::cout << pos->id << " " << pos->name << std::endl;
      if(pos != &s1) {
          list_del(&pos->node);
          delete pos;
      }
  }
  assert(list_is_empty(&s1.node));
  */
}

{

  struct list_head head1, head2;
  list_inithead(&head1);
  list_inithead(&head2);

  for (int i = 0; i < 10; i++) {
    student *s1 = new student{i, "Tony" + std::to_string(i), {}};
    list_addtail(&s1->node, &head1);

    student *s2 = new student{i + 33, "Alice" + std::to_string(i), {}};
    list_addtail(&s2->node, &head2);
  }

  // std::cout << list_length(&head1); // 10
  // std::cout << list_length(&head2); // 10
  // splice是插入到head2节点的后面，假如head2是哨兵节点，那么相当于插入到head2链表的头部
  // list_splice(&head1,&head2);
  // splice tail只是插入到前面，对于循环链表，插入到哨兵节点前面相当于尾插
  list_splicetail(&head1,&head2);
  list_inithead(&head1);
  std::cout << list_length(&head1); // 0
  std::cout << list_length(&head2); // 20

  list_for_each_entry_safe(student, pos, &head2, node) {
      std::cout << pos->id << " " << pos->name << std::endl;
  }

}
  return 0;
}