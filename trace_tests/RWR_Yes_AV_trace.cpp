/* C program with just calls to atomicity violation detection library*/

#include "t_debug_task.h"

int x = 0;
 
int main( int argc, const char *argv[] ) {
  TD_Activate();

  taskGraph->CaptureSetTaskId(0, 1, true);
  taskGraph->CaptureExecute(1, 1);
  CaptureExecute(1);
  RecordMem(1, &x, READ);
  RecordMem(1, &x, READ);
  taskGraph->CaptureReturn(1);
  CaptureReturn(1);

  taskGraph->CaptureSetTaskId(0, 2, true);
  taskGraph->CaptureExecute(2, 2);
  CaptureExecute(2);
  RecordMem(2, &x, WRITE);
  taskGraph->CaptureReturn(2);
  CaptureReturn(2);

  Fini();
}
