#include <iostream>
#include "memory_pool.h"

using namespace std;

struct tt {
  int i;
  int j;
};

int main() {
  
  CMemPool* m = &CMemPool::get_instance();
  
  char a[] = "hello";
  char b[] = "world";

  char* p = (char*) m->alloc(2000);
  char* q = (char*) m->alloc(2000);
  strcpy(p, a);
  strcpy(q, b);
  cout << p << endl << q << endl;
  m->stats();
  cout << "////" << endl;

  m->free(p);
  cout << p << endl << q << endl;
  m->stats();
  cout << "////" << endl;

  p = (char*) m->alloc(2000);
  strcpy(p, b);
  m->stats();
  cout << "////" << endl;
  cout << p << endl << q << endl;

  tt* t1 = (tt*) CMemPool::get_instance().alloc(sizeof(tt) * 2); //16 bytes
  tt* t2 = (tt*) m->alloc(sizeof(tt) * 5); //40
  t1[0].i = 10;
  t1[0].j = 20;
  t1[1].i = 30;
  t1[1].j = 40;
  m->stats();
  cout << "////" << endl;

  m->free(t2);
  m->stats();
  cout << "////" << endl;

  CMemPool::get_instance().ratio(); //nice way!
  return 0;
}
