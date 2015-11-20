#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "tbb/parallel_for.h"
#include <math.h>
//#include <iostream>

using namespace tbb;

#define TIMER_T                         struct timeval

#define TIMER_READ(time)                gettimeofday(&(time), NULL)

#define TIMER_DIFF_SECONDS(start, stop) \
  (((double)(stop.tv_sec)  + (double)(stop.tv_usec / 1000000.0)) - \
   ((double)(start.tv_sec) + (double)(start.tv_usec / 1000000.0)))

#define ARRAY_SIZE 100000

class ApplyFoo {
  float *const my_a;
public:
  void operator()( const blocked_range<size_t>& r, size_t taskId ) const {
    float *a = my_a;
    for( size_t i=r.begin(); i!=r.end(); ++i ) { 
      a[i] = sqrt(a[i]);
      a[i] += 23;
      a[i] *= 34;
      a[i] -= 234;
    }
  }
  ApplyFoo( float a[] ) :
  my_a(a)
  {}
};

void ParallelApplyFoo( float a[], size_t n ) {
  parallel_for(blocked_range<size_t>(0,n), ApplyFoo(a));
}

int main(int argc, char** argv) {
  TD_Activate();
  //float a[ARRAY_SIZE];
  TIMER_T startTime1;
  TIMER_READ(startTime1);

  float *a = (float*)malloc (ARRAY_SIZE*sizeof(float));

  for (size_t i=0; i< ARRAY_SIZE;i++) {
    a[i] = rand() % 1000;
  }

  ParallelApplyFoo(a, ARRAY_SIZE);
  
  TIMER_T stopTime1;
  TIMER_READ(stopTime1);

  Fini();
  //printf("TBB time = %f\n", TIMER_DIFF_SECONDS(startTime1, stopTime1));

  /*for (size_t i=0; i< ARRAY_SIZE;i++) {
    printf("a[%lu] = %f\n", i, a[i]);
    }*/
}
