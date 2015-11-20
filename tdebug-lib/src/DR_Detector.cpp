#ifdef DR_ANALYSIS

#include "DR_Detector.H"
#include "AFTask.H"

// 2^23 entries each will be 8 bytes each
const size_t SS_PRIMARY_TABLE_ENTRIES = ((size_t) 8*(size_t) 1024 * (size_t) 1024);

// each secondary entry has 2^ 22 entries (32 bytes each)
const size_t SS_SEC_TABLE_ENTRIES = ((size_t) 4*(size_t) 1024 * (size_t) 1024);

struct Dr_Address_Data** shadow_space;

std::ofstream report;

static void report_access(size_t a, AccessType a_type) {
  report << a << "          ";
  if (a_type == READ)
    report << "READ\n";
  else
    report << "WRITE\n";
}

static void report_race(ADDRINT addr, size_t a1, AccessType a1_type, size_t a2, AccessType a2_type) {
  report << "** Data race detected at " << addr << " **\n";
  report << "Accesses:\n";
  report << "TaskId    AccessType\n";
  report_access(a1, a1_type);
  report_access(a2, a2_type);
  report << "*******************************\n";
}

extern "C" void TD_Activate() {
  taskGraph = new AFTaskGraph();

  size_t primary_length = (SS_PRIMARY_TABLE_ENTRIES) * sizeof(struct Dr_Address_Data*);
  shadow_space = (struct Dr_Address_Data**)mmap(0, primary_length, PROT_READ| PROT_WRITE,
		      MMAP_FLAGS, -1, 0);
  assert(shadow_space != (void *)-1);
  report.open("data_races.out");
}

bool exceptions (THREADID threadid, ADDRINT addr) {
  return (tidToTaskIdMap[threadid].empty() || 
	  tidToTaskIdMap[threadid].top() == 0
	  );
}

extern "C" void RecordMem(THREADID threadid, void * addr, AccessType accessType) {
  PIN_GetLock(&lock, 0);

  ADDRINT addr_addr = (ADDRINT) addr;

  // Exceptions
  if(exceptions(threadid, addr_addr)){ 
    PIN_ReleaseLock(&lock);
    return;
  }

  //check for DR with previous accesses
  size_t primary_index = (addr_addr >> 25);
  struct Dr_Address_Data* primary_ptr = shadow_space[primary_index];
  if (primary_ptr != NULL) {
    size_t offset = (addr_addr >> 3) & 0x3fffff;
    struct Dr_Address_Data* addr_vec = primary_ptr + offset;
    if (addr_vec != NULL) {
      //if the tasks can execute in parallel
      if (addr_vec->wr_task != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->wr_task, threadid)) {
	//report race
	report_race(addr_addr, tidToTaskIdMap[threadid].top(), accessType, addr_vec->wr_task->taskId, WRITE);
      }
      if (accessType == WRITE) {
	if (addr_vec->r1_task != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->r1_task, threadid)) {
	  //report race
	  report_race(addr_addr, tidToTaskIdMap[threadid].top(), accessType, addr_vec->r1_task->taskId, READ);
	} else if (addr_vec->r2_task != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->r2_task, threadid)) {
	  //report race
	  report_race(addr_addr, tidToTaskIdMap[threadid].top(), accessType, addr_vec->r2_task->taskId, READ);
	}
      }
    }
  } else {
    //mmap secondary table
    size_t sec_length = (SS_SEC_TABLE_ENTRIES) * sizeof(struct Dr_Address_Data);
    primary_ptr = (struct Dr_Address_Data*)mmap(0, sec_length, PROT_READ| PROT_WRITE,
		      MMAP_FLAGS, -1, 0);
    shadow_space[primary_index] = primary_ptr;
  }

  //update shadow space
  size_t offset = (addr_addr >> 3) & 0x3fffff;
  struct Dr_Address_Data* addr_vec = primary_ptr + offset;
  if (accessType == WRITE) {
    addr_vec->wr_task = taskGraph->getCurTask(threadid);
  } else {
    //Update read
    if (addr_vec->r1_task == NULL) {
      addr_vec->r1_task = taskGraph->getCurTask(threadid);
      PIN_ReleaseLock(&lock);
      return;
    } else if (addr_vec->r2_task == NULL) {
      addr_vec->r2_task = taskGraph->getCurTask(threadid);
      PIN_ReleaseLock(&lock);
      return;
    }
    
    bool cur_r1 = taskGraph->areParallel(tidToTaskIdMap[threadid].top(),addr_vec->r1_task, threadid);
    bool cur_r2 = taskGraph->areParallel(tidToTaskIdMap[threadid].top(),addr_vec->r2_task, threadid);
    if (!cur_r1 && !cur_r2) {
      addr_vec->r1_task = taskGraph->getCurTask(threadid);
      addr_vec->r2_task = NULL;
    } else if (cur_r1 && cur_r2) {
      struct Task* curTask = taskGraph->getCurTask(threadid);
      struct AFTask* lca12 = static_cast<struct AFTask*>(taskGraph->LCA(addr_vec->r1_task, addr_vec->r2_task));
      struct AFTask* lca1s = static_cast<struct AFTask*>(taskGraph->LCA(addr_vec->r1_task, curTask));
      //struct AFTask* lca2s = static_cast<struct AFTask*>(taskGraph->LCA(addr_vec->r2_task, curTask));
      if (lca1s->depth < lca12->depth /*|| lca2s->depth < lca12->depth*/)
	addr_vec->r1_task = curTask;
    }
  }

  PIN_ReleaseLock(&lock);
}

extern "C" void Fini()
{
  //std::cout << "Size of shadow space = " << shadow_space.size() << " max size = " << shadow_space.max_size() << std::endl;
  report.close();
}

void CaptureExecute(THREADID threadid) {}

void CaptureReturn(THREADID threadid) {}
#endif
