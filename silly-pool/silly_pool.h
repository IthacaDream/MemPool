#ifndef __SILLY_POOL_H__
#define __SILLY_POOL_H__

#include <iostream>
#include <stdlib.h>

class SillyPool {

public:

  SillyPool(int block_size, int block_num);
  ~SillyPool();
  void* alloc();
  void free(void* ptr);

  void stat() const;
  
private:
  SillyPool();
  SillyPool(const SillyPool&);
  SillyPool& operator= (const SillyPool&);

  int block_size_;
  int block_num_;
  void* base_addr_;
  int* free_list_;
  int curr_free_;

};

#endif

