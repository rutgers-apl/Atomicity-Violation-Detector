#ifdef DR_ANALYSIS

#ifndef DR_DETECTOR_H
#define DR_DETECTOR_H

#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sys/mman.h>

#include "Common.H"
#include "Task.H"
#include "AFTaskGraph.H"

#define MMAP_FLAGS (MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE)

struct Dr_Address_Data {
private:
public:
  struct Task* wr_task;
  struct Task* r1_task;
  struct Task* r2_task;

  Dr_Address_Data() {
    wr_task = r1_task = r2_task = NULL;
  }
};

extern AFTaskGraph* taskGraph;

//private functions
// static bool exceptions(THREADID threadid, ADDRINT addr);
// static void report_access(size_t a, AccessType a_type);
// static void report_race(ADDRINT addr, size_t a1, AccessType a1_type, size_t a2, AccessType a2_type);

//public functions
extern "C" void TD_Activate();
extern "C" void RecordMem(THREADID threadid, void * addr, AccessType accessType);
extern "C" void Fini();
void CaptureExecute(THREADID threadid);
void CaptureReturn(THREADID threadid);

#endif
#endif
