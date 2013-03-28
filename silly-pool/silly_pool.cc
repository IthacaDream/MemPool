#include "silly_pool.h"

SillyPool::SillyPool(int block_size, int block_num) :
  block_size_(block_size), 
  block_num_(block_num),
  curr_free_(0) {
  
  try {
    base_addr_ = new char[block_size_ * block_num_];
    free_list_ = new int[block_num_];
  } catch(const std::bad_alloc &e) {
    std::cerr<<"bad_alloc: "<<e.what()<<std::endl;
    exit(1);
  }

  for (int i = 0; i < block_num_; ++i) {
    free_list_[i] = i;
  }

}

SillyPool::~SillyPool() {
  delete [](char*)base_addr_;
  delete []free_list_;
}

void* SillyPool::alloc() {
  if (curr_free_ == block_num_)
    return NULL;
  int available = free_list_[curr_free_++];
  return (char*)base_addr_ + available  * block_size_; 
}

void SillyPool::free(void* ptr) {
  int block_id = ((char*)ptr - (char*)base_addr_) / block_size_;
  --curr_free_;
  if  (curr_free_ < 0) { // multi-free
    std::cerr<<"free error"<<std::endl;
  }
  free_list_[curr_free_] = block_id;
}

void SillyPool::stat() const {
  std::cout<<"total block size: "<<block_num_
	   <<"\tbytes: "<<block_size_ * block_num_
	   <<std::endl;
  std::cout<<"already used: "<<curr_free_<<" blocks."
	   <<std::endl;
}

//test
int main() {
  
  SillyPool* pool = new SillyPool(sizeof(int),5);

  pool->stat();

  int* p = (int*)pool->alloc();
  *p=13;
  int* q = (int*)pool->alloc();
  *q=14;
  std::cout<<*p<<std::endl
	   <<*q<<std::endl;

  pool->stat();

  pool->free(p);
  p = NULL;
  p = (int*)pool->alloc();

  pool->stat();
  
  pool->free(q);
  pool->free(p);

  pool->stat();
  delete pool;
  return 0;
}
