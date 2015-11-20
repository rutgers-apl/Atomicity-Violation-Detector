#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "tbb/parallel_for.h"
#include <math.h>

using namespace tbb;

#define ARRAY_SIZE 100000000

size_t CHECK_AV ctr = 0;

class ApplyFoo {
  float *const my_a;
public:
  void operator()( const blocked_range<size_t>& r, size_t thdId ) const {
    //RecordMem(thdId, &ctr, READ);
    //RecordMem(thdId, &ctr, WRITE);
    ctr++;
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
  float *a = (float*)malloc (ARRAY_SIZE*sizeof(float));

  for (size_t i=0; i< ARRAY_SIZE;i++) {
    a[i] = rand() % 1000;
  }

  ParallelApplyFoo(a, ARRAY_SIZE);
  
  printf("Counter = %lu addr = %lu\n", ctr, (size_t)&ctr);

  Fini();
  /*for (size_t i=0; i< ARRAY_SIZE;i++) {
    printf("a[%lu] = %f\n", i, a[i]);
    }*/
}
