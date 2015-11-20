//#include <iostream>
#include "t_debug_task.h"

using namespace std;
using namespace tbb;

int CHECK_AV x = 0;

class SimpleAV: public t_debug_task {
public:
  task* execute() {
    __exec_begin__(getTaskId());

    // READ & WRITE
    //RecordMem(get_cur_tid(), &x, READ);
    //RecordMem(get_cur_tid(), &x, WRITE);
    x++;

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

  for (int i = 0 ; i < 10000000 ; i++);
  //cout << "x " << (size_t)&x << " is " << x << std::endl;

  Fini();
}
