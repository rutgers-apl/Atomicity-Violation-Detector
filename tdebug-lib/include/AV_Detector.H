#ifdef AV_ANALYSIS

#ifndef AV_DETECTOR_H
#define AV_DETECTOR_H

#include <assert.h>
#include <iostream>
#include <fstream>

#include "Lockset_data.H"
#include "Address_data.H"
#include "Common.H"
#include "AFTaskGraph.H"
#include <stdlib.h>
#include <cstring>

extern AFTaskGraph* taskGraph;

struct violation {
  struct Address_data* a1;
  struct Address_data* a2;
  struct Address_data* a3;
  
  violation(Address_data* a1,
	    Address_data* a2,
	    Address_data* a3) {
    this->a1 = a1;
    this->a2 = a2;
    this->a3 = a3;
  }
};

struct global_access_hist {
  struct Address_data* single_read;
  struct Address_data* single_write;
  struct Address_data* single_write_1;
  struct Address_data* rr1;
  struct Address_data* rr2;
  struct Address_data* rw1;
  struct Address_data* rw2;
  struct Address_data* wr1;
  struct Address_data* wr2;
  struct Address_data* ww1;
  struct Address_data* ww2;
};

struct local_access_hist {
  struct Address_data* read_addr_data;
  struct Address_data* write_addr_data;
};

//public functions
extern "C" void TD_Activate();

void CaptureExecute(THREADID threadid);
void CaptureReturn(THREADID threadid);
extern "C" void RecordMem(THREADID threadid, void * addr, AccessType accessType);
  
void CaptureLockAcquire(THREADID threadid, ADDRINT lock_addr);
void CaptureLockRelease(THREADID threadid);
void CaptureLockFnBegin(THREADID threadid);
void CaptureLockFnEnd(THREADID threadid);
void CaptureAtomicSet(ADDRINT* addr, size_t size, ADDRINT addr_temp);
extern "C" void Fini();

#endif
#endif
