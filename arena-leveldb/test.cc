#include <iostream>
#include "arena.h"

using namespace std;

int main() {

    Arena arena;
    size_t bytes = 1024;
    arena.Allocate(bytes);
    arena.AllocateAligned(bytes/4);
    cout<<"use: "<<arena.MemoryUsage()<<" Bytes"<<endl;
    
    return 0;
}
