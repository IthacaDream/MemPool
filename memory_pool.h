#ifndef MEMPOOL_H_
#define MEMPOOL_H_

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

const int POWER_SMALLEST = 1;
const int POWER_LARGEST = 20;   // 2^20 Byte = 1 M
const int POWER_BLOCK = 1048576; //default 1M=1024*1024
const int CHUNK_ALIGN_BYTES = 4; //maybe 8

class CMemPool {

public:
  static CMemPool& get_instance();

  //thread-safe version
  void* alloc(const size_t size);
  void free(void *ptr);
  void stats() const;
  void ratio() const;

  //non-thread version
  void* do_slabs_alloc(const size_t size);
  void do_slabs_free(void *ptr);
  void do_stats() const;
  void get_ratio() const;

  void release() { delete instance_; }
private:

  unsigned int get_slab_id(const size_t size) const;

  void init_slabs(const size_t limit = 0, const double factor = 2.0,
      const size_t start_size = 8); //shoud let limit=0

  int new_slab(const unsigned int id);

  void* allocate_memory(size_t size);

  int grow_slab_list(const unsigned int id);

  size_t get_rss() const;

private:
  CMemPool();
  CMemPool(const CMemPool&);
  CMemPool& operator= (const CMemPool&);
  ~CMemPool();
  static CMemPool* instance_;

private:
  typedef struct {
    unsigned int vsize;          /* virtual vsize=size+psize (psize is the pointer size)*/
    unsigned int size;           /* sizes of chunk */
    unsigned int chunks_num;     /* how many chunks per slab */

    void **free_chunks_list;     /* list of free chunks(after free)*/
    unsigned int fc_total;       /* size of free_chunks_list */
    unsigned int fc_curr;        /* current free chunks */

    void *end_page_ptr;          /* pointer to next free chunk at the end of page, or NULL */
    unsigned int end_page_free;  /* number of chunk remaining at the end of last alloced page */

    unsigned int slabs_curr;     /* current slabs num*/

    void **slab_list;            /* list of slab pointers */
    unsigned int slab_list_size; /* size of slabs array */
    size_t requested;            /*the number of requested bytes*/
  } slabclass_t;

private:

  slabclass_t slabclass[POWER_LARGEST + 1];
  int power_largest;
  mutable pthread_mutex_t slabs_lock; //access to slab is protected by this lock

  static unsigned int psize; //reserve to record pointer size
  static size_t mem_limit;
  static size_t mem_malloced; //record how many bytes we malloc
};

#endif
