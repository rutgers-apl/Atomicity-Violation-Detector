#include "t_debug_task.h"
#include "tbb/mutex.h"
#include<iostream>

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

    // READ
    CaptureLockAcquire(get_cur_tid(), (ADDRINT)&x_mutex);
    myLock.acquire(x_mutex);    
    if (x == 1) {
      CaptureLockRelease(get_cur_tid());
      myLock.release();    
      set_ref_count(3);
      t_debug_task& a = *new(allocate_child()) SimpleAV();
      spawn(a);
      cout << "child a task id = " << a.getTaskId() << endl;
      t_debug_task& b = *new(allocate_child()) SimpleAV();
      spawn(b);
      cout << "child b Task id = " << b.getTaskId() << endl;
      wait_for_all ();
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
  
  t_debug_task& a = *new(task::allocate_root()) SimpleAV();
  t_debug_task::spawn_root_and_wait(a);
  cout << "root a Task id = " << a.getTaskId() << endl;
  t_debug_task& b = *new(task::allocate_root()) SimpleAV();
  t_debug_task::spawn_root_and_wait(b);
  cout << " root b Task id = " << b.getTaskId() << endl;
  //for (int i = 0; i < 100000000 ; i++);
  cout << "x " << (size_t)&x << " is " << x << std::endl;

  Fini();
}
