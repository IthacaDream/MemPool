#include "memory_pool.h"

pthread_mutex_t imutex = PTHREAD_MUTEX_INITIALIZER;

CMemPool* CMemPool::instance_ = NULL;
unsigned int CMemPool::psize = 4; //bytes that reserved to record pointer size
size_t CMemPool::mem_limit = 0; //not use now
size_t CMemPool::mem_malloced = 0; //record how many bytes we have malloced

CMemPool::CMemPool() {
  pthread_mutex_init(&slabs_lock, NULL);
  init_slabs(0, 2.0, 8);
}

CMemPool::~CMemPool() {
  //TODO
  pthread_mutex_destroy(&slabs_lock);

  //it's time that we should really free all the mem-space(all slabs) we used.
  for (int i = POWER_SMALLEST; i <= power_largest; i++) {
    const slabclass_t *p = &slabclass[i];
    unsigned int p_curr = p->slabs_curr;
    while (p_curr--) {
      free(p->slab_list[p_curr]);
    }
    if (p->slab_list) //not need
      free(p->slab_list);
    if (p->free_chunks_list)
      free(p->free_chunks_list);

  }
}

CMemPool& CMemPool::get_instance() {
  if (!instance_) { //Double-Checked Locking
    pthread_mutex_lock(&imutex);
    if (!instance_)
      instance_ = new CMemPool;
    pthread_mutex_unlock(&imutex);
  }
  return *instance_;
}

void CMemPool::init_slabs(const size_t limit, const double factor,
    const size_t start_size) {

  int i = POWER_SMALLEST - 1;
  unsigned int vsize = start_size;
  if (factor == 2.0 && vsize < 8)
    vsize = 8;

  mem_limit = limit; //TODO

  memset(slabclass, 0, sizeof(slabclass)); //set 0

  while (++i < POWER_LARGEST && vsize <= POWER_BLOCK / 2) {
    if (vsize % CHUNK_ALIGN_BYTES) //TODO; now only psize-align
      vsize += CHUNK_ALIGN_BYTES - (vsize % CHUNK_ALIGN_BYTES);

    slabclass[i].vsize = vsize;
    slabclass[i].size = vsize - psize; //so,waste psize bytes
    slabclass[i].chunks_num = POWER_BLOCK / slabclass[i].vsize; //now,vsize=chunk size
    vsize *= factor; //use vsize to avoid space-wasting
  }

  power_largest = i;
  slabclass[power_largest].vsize = POWER_BLOCK; //TODO
  slabclass[power_largest].size = vsize - psize;
  slabclass[power_largest].chunks_num = 1; //but size<POWER_BLOCK
}

unsigned int CMemPool::get_slab_id(const size_t size) const {
  int res = POWER_SMALLEST;

  if (size == 0)
    return 0;
  while (size > slabclass[res].size)
    if (res++ == power_largest) /* won't fit in the biggest slab */
      return 0;
  return res;
}

int CMemPool::grow_slab_list(const unsigned int id) {
  slabclass_t *p = &slabclass[id];
  if (p->slabs_curr == p->slab_list_size) {
    size_t new_size = (p->slab_list_size != 0) ? p->slab_list_size * 2 : 16; //TODO maybe not 16
    void **new_list = (void**) realloc(p->slab_list, new_size * sizeof(void *)); //TODO
    if (new_list == 0)
      return 0;
    p->slab_list_size = new_size;
    p->slab_list = new_list;
  }
  return 1;
}

int CMemPool::new_slab(const unsigned int id) {
  slabclass_t *p = &slabclass[id];
  int len = p->vsize * p->chunks_num;
  char *ptr;

  if ((mem_limit && mem_malloced + len > mem_limit && p->slabs_curr > 0)
      || //TODO
      (grow_slab_list(id) == 0)
      || ((ptr = (char*) allocate_memory((size_t) len)) == 0)) {
    return 0;
  }

  //memset(ptr, 0, (size_t)len);
  p->end_page_ptr = ptr;
  p->end_page_free = p->chunks_num;

  p->slab_list[p->slabs_curr++] = ptr;
  mem_malloced += len;

  return 1;
}

void* CMemPool::do_slabs_alloc(const size_t size) {
  slabclass_t *p;
  void *ret = NULL;
  unsigned int id = get_slab_id(size);
  if (id < POWER_SMALLEST || id > power_largest) {
    return NULL;
  }

  p = &slabclass[id];

  /* fail unless we have space at the end of a recently allocated page,
   we have something on our freelist, or we could allocate a new page */
  if (!(p->end_page_ptr != 0 || p->fc_curr != 0 || new_slab(id) != 0)) {
    /* We don't have more memory available */
    ret = NULL;
  } else if (p->fc_curr != 0) {
    /* return off our freelist */
    ret = p->free_chunks_list[--p->fc_curr];
  } else {
    assert(p->end_page_ptr != NULL);
    //TODO; we shoud better thow exceptions
    ret = p->end_page_ptr;
    if (--p->end_page_free != 0) {
      p->end_page_ptr = (char*) p->end_page_ptr + p->vsize;
    } else {
      p->end_page_ptr = 0;
    }
  }

  if (ret) {
    p->requested += size; //record requested bytes
  } else {
    return NULL;
  }

  //record the point size,so we can locate it when do_slabs_free.
  *(unsigned int*) ret = size; //make sure it compatible on 32-bit and 64-bit machine

  return (char*) ret + psize;
}

void* CMemPool::allocate_memory(size_t size) { //TODO
  void *ret = malloc(size);
  return ret;
}

void CMemPool::do_slabs_free(void *ptr) { //because it used on a server,we need never really free.
  slabclass_t *p;

  const size_t size = *(unsigned int*) ((char*) ptr - psize);
  ; //get the real ptr size

  unsigned int id = get_slab_id(size);

  assert(id >= POWER_SMALLEST && id <= power_largest);
  //abort
  if (id < POWER_SMALLEST || id > power_largest)
    return;

  p = &slabclass[id];

  if (p->fc_curr == p->fc_total) { /* need more space on the free list */
    int new_size = (p->fc_total != 0) ? p->fc_total * 2 : 16;
    void **new_free_chunks_list = (void**) realloc(p->free_chunks_list,
        new_size * sizeof(void *));
    if (new_free_chunks_list == 0)
      return;
    p->free_chunks_list = new_free_chunks_list;
    p->fc_total = new_size;
  }
  p->free_chunks_list[p->fc_curr++] = (char*) ptr - psize; //en...
  p->requested -= size;
  return;
}

void CMemPool::do_stats() const {
  int total(0);
  for (int i = POWER_SMALLEST; i <= power_largest; i++) {
    const slabclass_t *p = &slabclass[i];
    if (p->slabs_curr != 0) {
      size_t slabs = p->slabs_curr;
      size_t chunks_num = p->chunks_num;
      printf("STAT %d:chunk_size %u\r\n", i, p->size);
      printf("STAT %d:chunk_vsize %u\r\n", i, p->vsize);
      printf("STAT %d:chunks_per_page %u\r\n", i, chunks_num);
      printf("STAT %d:total_pages %u\r\n", i, slabs);
      printf("STAT %d:total_chunks %u\r\n", i, slabs * chunks_num);
      printf("STAT %d:used_chunks %u\r\n", i,
          slabs * chunks_num - p->fc_curr - p->end_page_free);
      printf("STAT %d:free_chunks_list %u\r\n", i, p->fc_curr);
      printf("STAT %d:free_chunks_list_end %u\r\n", i, p->end_page_free);
      total++;
    }
  }
  printf("STAT active_slabs %d\r\nSTAT total_malloced %llu\r\n", total,
      (unsigned long long) mem_malloced);
  printf("-------------------END----------------\r\n");
}

void CMemPool::get_ratio() const {
  unsigned int /*4 critical parammeters to measure the performance*/
  total_requested(0), //the real requested memory size
  total_used(0), //the size we alloced (total non-free chunks size)
  total_malloced(0), //all of the malloced size
  rss(0); //memory used by this program

  for (int i = POWER_SMALLEST; i <= power_largest; i++) {
    const slabclass_t *p = &slabclass[i];
    if (p->slabs_curr != 0) {
      size_t slabs = p->slabs_curr;
      size_t chunks_num = p->chunks_num;
      total_requested += p->requested;
      total_used += (slabs * chunks_num - p->fc_curr - p->end_page_free)
          * p->vsize;
    }
  }
  total_malloced = mem_malloced;
  rss = get_rss();
  if (rss == 0) {
    cout << "getRSS error" << endl;
    return;
  }

  cout << "--------stat & ratio--------" << endl;
  cout << "total_requested(R): " << total_requested << endl << "total_used(U): "
      << total_used << endl << "total_malloced(M): " << total_malloced << endl
      << "rss(T): " << rss << endl;

  cout << "R/U=" << (float) total_requested / total_used << endl << "U/M="
      << (float) total_used / total_malloced << endl << "M/T="
      << (float) total_malloced / rss << endl;
}

size_t CMemPool::get_rss() const {
  int page = sysconf(_SC_PAGESIZE); //get page size
  //cout<<"page:"<<page<<endl;
  size_t rss;
  char buf[4096];
  char filename[256];
  int fd, count;
  char *p, *x;

  snprintf(filename, 256, "/proc/%d/stat", getpid());
  if ((fd = open(filename, O_RDONLY)) == -1)
    return 0;
  if (read(fd, buf, 4096) <= 0) {
    close(fd);
    return 0;
  }
  close(fd);

  p = buf;
  count = 23; /* page_num is the 24th field in /proc/<pid>/stat */
  while (p && count--) {
    p = strchr(p, ' ');
    if (p)
      p++;
  }
  if (!p)
    return 0;
  x = strchr(p, ' ');
  if (!x)
    return 0;
  *x = '\0'; //end

  rss = strtoll(p, NULL, 10); //get memory page num
  rss *= page; //compute rss(Resident Set Size)
  return rss; //bytes
}

void* CMemPool::alloc(const size_t size) {
  void* ret;
  pthread_mutex_lock(&slabs_lock);
  ret = do_slabs_alloc(size);
  pthread_mutex_unlock(&slabs_lock);
  return ret;
}

void CMemPool::free(void *ptr) {
  pthread_mutex_lock(&slabs_lock);
  do_slabs_free(ptr);
  pthread_mutex_unlock(&slabs_lock);
}

void CMemPool::stats() const {
  pthread_mutex_lock(&slabs_lock);
  do_stats();
  pthread_mutex_unlock(&slabs_lock);
}

void CMemPool::ratio() const {
  pthread_mutex_lock(&slabs_lock);
  get_ratio();
  pthread_mutex_unlock(&slabs_lock);
}
