//#include<iostream>
#include "t_debug_task.h"
#include "tbb/mutex.h"

using namespace std;
using namespace tbb;

tbb::mutex x_mutex;
tbb::mutex::scoped_lock myLock;

int CHECK_AV x = 0;

class SimpleAV: public t_debug_task {
public:
  task* execute() {
    __exec_begin__(getTaskId());

    // READ & WRITE
    CaptureLockAcquire(get_cur_tid(), (ADDRINT)&x_mutex);
    myLock.acquire(x_mutex);
    
    x++;

    CaptureLockRelease(get_cur_tid());
    myLock.release();

    __exec_end__(getTaskId());
    return NULL;
  }
};

int main( int argc, const char *argv[] ) {
  TD_Activate();
  
  task& a = *new(task::allocate_root()) SimpleAV();
  t_debug_task::spawn(a);
  
  task& b = *new(task::allocate_root()) SimpleAV();
  t_debug_task::spawn(b);
  
  for (int i = 0 ; i < 100000000 ; i++);
  //cout << "x " << (size_t)&x << " is " << x << std::endl;
  
  Fini();
}
