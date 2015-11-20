#include "t_debug_task.h"
#include<iostream>
#include "AV_Detector.H"
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
    
    //RecordMem(get_cur_tid(), &x, READ);
    //RecordMem(get_cur_tid(), &x, WRITE);
    x++;

    // READ
    //RecordMem(get_cur_tid(), &x, READ);
    if (x == 1) {
      CaptureLockRelease(get_cur_tid());
      myLock.release();
      //set_ref_count(3);
      //task& a = *new(allocate_child()) SimpleAV();
      //spawn(a);

      CaptureLockAcquire(get_cur_tid(), (ADDRINT)&x_mutex);
      myLock.acquire(x_mutex);
      //RecordMem(get_cur_tid(), &x, READ);
      //RecordMem(get_cur_tid(), &x, WRITE);
      x++;

      CaptureLockRelease(get_cur_tid());
      myLock.release();

      //task& b = *new(allocate_child()) SimpleAV();
      //spawn_and_wait_for_all(b);
    } else {
      CaptureLockRelease(get_cur_tid());
      myLock.release();
    }

    __exec_end__(getTaskId());
    return NULL;
  }
};
 
int main( int argc, const char *argv[] ) {
  TD_Activate();
  task& a = *new(task::allocate_root()) SimpleAV();
  t_debug_task::spawn(a);
  task& b = *new(task::allocate_root()) SimpleAV();
  t_debug_task::spawn_root_and_wait(b);
  cout << "x " << (size_t)&x << " is " << x << std::endl;
  for (int i = 0 ; i < 10000000 ; i++);
  Fini();
}
