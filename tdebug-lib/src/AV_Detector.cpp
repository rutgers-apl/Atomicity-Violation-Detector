#ifdef AV_ANALYSIS

#include <sys/mman.h>
#include "AV_Detector.H"

//#define MMAP_FLAGS (MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE)

// 2^10 entries each will be 8 bytes each
const size_t SS_PRIMARY_TABLE_ENTRIES = ((size_t) 1024);

// each secondary entry has 2^ 22 entries (64 bytes each)
const size_t SS_SEC_TABLE_ENTRIES = ((size_t) 4*(size_t) 1024 * (size_t) 1024);

//array of stacks of lockset_data
std::stack<struct Lockset_data*> thd_lockset[NUM_THREADS];

//std::stack<std::map< ADDRINT, struct access_array>*> AV_Detector::access_shadow[NUM_THREADS];
//std::stack<std::map< ADDRINT, struct local_access_hist>*> local_shadow_space[NUM_THREADS];
std::stack< struct local_access_hist** > local_shadow_space[NUM_THREADS];

struct global_access_hist** shadow_space;
//std::map< ADDRINT, struct global_access_hist > shadow_space;

std::stack<bool> lock_fn[NUM_THREADS];

std::ofstream report;

std::map<ADDRINT, ADDRINT> atomic_sets;

std::map<ADDRINT, struct violation*> all_violations;

static void report_access(struct Address_data* a) {
  report << a->task->taskId << "          ";
  if (a->accessType == READ)
    report << "READ\n";
  else
    report << "WRITE\n";
}

static void report_AV(ADDRINT addr, struct Address_data* a1, struct Address_data* a2, struct Address_data* a3) {
  report << "** Atomicity violation detected at " << addr << " **\n";
  report << "Accesses:\n";
  report << "TaskId    AccessType\n";
  report_access(a1);
  report_access(a2);
  report_access(a3);
  report << "*******************************\n";
}

extern "C" void TD_Activate() {
  taskGraph = new AFTaskGraph();

  size_t primary_length = (SS_PRIMARY_TABLE_ENTRIES) * sizeof(struct global_access_hist*);
  shadow_space = (struct global_access_hist**)mmap(0, primary_length, PROT_READ| PROT_WRITE,
						MMAP_FLAGS, -1, 0);
  assert(shadow_space != (void *)-1);
  
  thd_lockset[0].push(new Lockset_data());
}

void CaptureLockAcquire(THREADID threadid, ADDRINT lock_addr) {
  //PIN_GetLock(&lock, 0);

  struct Lockset_data* curLockset = thd_lockset[threadid].top();

  //IF: lock is present in lockset ERROR
  assert(!curLockset->isLockPresent(lock_addr));

  //ELSE: Add to lockset
  curLockset->addLockToLockset(lock_addr);

  curLockset->updateVersionNumber(lock_addr);

  //PIN_ReleaseLock(&lock);
}

void CaptureLockRelease(THREADID threadid) {
  //PIN_GetLock(&lock, 0);

  struct Lockset_data* curLockset = thd_lockset[threadid].top();
  curLockset->removeLockFromLockset();

  //PIN_ReleaseLock(&lock);
}

void CaptureLockFnBegin(THREADID threadid) {
  //PIN_GetLock(&lock, 0);
  lock_fn[threadid].top() = false;
  //PIN_ReleaseLock(&lock);
}

void CaptureLockFnEnd(THREADID threadid) {
  //PIN_GetLock(&lock, 0);
  lock_fn[threadid].top() = true;
  //PIN_ReleaseLock(&lock);
}

void CaptureAtomicSet(ADDRINT* addr, size_t size, ADDRINT addr_temp) {
  PIN_GetLock(&lock, 0);

  ADDRINT* addrs = (ADDRINT*)malloc(size);
  memcpy(addrs, addr, size);

  ADDRINT value = addrs[0];

  for (unsigned int i = 0; i < size/sizeof(size_t);i++) {
    assert(atomic_sets.count(addrs[i]) == 0);
    atomic_sets.insert( std::pair<ADDRINT, ADDRINT>(addrs[i], value) );
  }

  PIN_ReleaseLock(&lock);
}

void CaptureExecute(THREADID threadid) {
  //PIN_GetLock(&lock, 0);

  thd_lockset[threadid].push(new Lockset_data());
  //access_shadow[threadid].push(new std::map<ADDRINT, struct access_array>());
  //local_shadow_space[threadid].push(new std::map<ADDRINT, struct local_access_hist>());

  size_t primary_length = (SS_PRIMARY_TABLE_ENTRIES) * sizeof(struct local_access_hist*);
  struct local_access_hist** local_ptr = (struct local_access_hist**)mmap(0, primary_length, PROT_READ| PROT_WRITE,
						MMAP_FLAGS, -1, 0);
  assert(local_ptr != (void *)-1);
  local_shadow_space[threadid].push(local_ptr);

  lock_fn[threadid].push(true);

  //PIN_ReleaseLock(&lock);
}

void CaptureReturn(THREADID threadid) {
  //PIN_GetLock(&lock, 0);
  struct Lockset_data* curLockset = thd_lockset[threadid].top();
  thd_lockset[threadid].pop();
  delete(curLockset);
  //std::map<ADDRINT, struct access_array>* thd_access_map = access_shadow[threadid].top();
  //access_shadow[threadid].pop();
  struct local_access_hist** thd_access_map = local_shadow_space[threadid].top();
  local_shadow_space[threadid].pop();
  for (size_t i = 0; i < SS_PRIMARY_TABLE_ENTRIES;i++) {
    if (thd_access_map[i] != NULL)
      munmap(thd_access_map[i], (SS_SEC_TABLE_ENTRIES) * sizeof(struct local_access_hist));
  }
  munmap(thd_access_map, (SS_PRIMARY_TABLE_ENTRIES) * sizeof(struct local_access_hist*));

  lock_fn[threadid].pop();

  //PIN_ReleaseLock(&lock);
}

static bool exceptions (THREADID threadid, ADDRINT addr) {
  return (tidToTaskIdMap[threadid].empty() || 
	  tidToTaskIdMap[threadid].top() == 0 || 
	  thd_lockset[threadid].empty() ||
	  /*addr > 100000000000000 ||*/
	  lock_fn[threadid].top() == false ||
	  all_violations.count(addr) != 0
	  ); 
}

static bool check_single_read(struct Address_data* sr_addr_data, global_access_hist& g_access_hist, ADDRINT addr, THREADID threadid) {
  if(g_access_hist.ww1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.ww1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(sr_addr_data, g_access_hist.ww1, g_access_hist.ww2)) );
    return true;
  }
  return false;
}

static bool check_single_write(struct Address_data* sw_addr_data, global_access_hist& g_access_hist, ADDRINT addr, THREADID threadid) {
  if(g_access_hist.rr1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.rr1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(sw_addr_data, g_access_hist.rr1, g_access_hist.rr2)) );
    return true;
  } else if(g_access_hist.rw1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.rw1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(sw_addr_data, g_access_hist.rw1, g_access_hist.rw2)) );
    return true;
  } else if(g_access_hist.wr1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.wr1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(sw_addr_data, g_access_hist.wr1, g_access_hist.wr2)) );
    return true;
  } else if (g_access_hist.ww1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.ww1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(sw_addr_data, g_access_hist.ww1, g_access_hist.ww2)) );
    return true;
  }
  return false;
}

static bool check_rr(global_access_hist& g_access_hist, ADDRINT addr, THREADID threadid) {
  if(g_access_hist.single_write != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write, g_access_hist.rr1, g_access_hist.rr2)) );
    return true;
  } else if(g_access_hist.single_write_1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write_1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write_1, g_access_hist.rr1, g_access_hist.rr2)) );
    return true;
  }
  return false;
}

static bool check_rw(global_access_hist& g_access_hist, ADDRINT addr, THREADID threadid) {
  if(g_access_hist.single_write != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write, g_access_hist.rw1, g_access_hist.rw2)) );
    return true;
  } else if(g_access_hist.single_write_1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write_1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write_1, g_access_hist.rw1, g_access_hist.rw2)) );
    return true;
  }
  return false;
}

static bool check_wr(global_access_hist& g_access_hist, ADDRINT addr, THREADID threadid) {
  if(g_access_hist.single_write != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write, g_access_hist.wr1, g_access_hist.wr2)) );
    return true;
  } else if(g_access_hist.single_write_1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write_1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write_1, g_access_hist.wr1, g_access_hist.wr2)) );
    return true;
  }
  return false;
}

static bool check_ww(global_access_hist& g_access_hist, ADDRINT addr, THREADID threadid) {
  if(g_access_hist.single_write != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write, g_access_hist.ww1, g_access_hist.ww2)) );
    return true;
  } else if(g_access_hist.single_read != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_read->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_read, g_access_hist.ww1, g_access_hist.ww2)) );
    return true;
  } else if(g_access_hist.single_write_1 != NULL && taskGraph->areParallel(tidToTaskIdMap[threadid].top(), g_access_hist.single_write_1->task, threadid)) {
    all_violations.insert( std::pair<ADDRINT, struct violation* >(addr, new violation(g_access_hist.single_write_1, g_access_hist.ww1, g_access_hist.ww2)) );
    return true;
  }
  return false;
}

static void update_access_histories(THREADID threadid, ADDRINT addr, AccessType accessType, struct Address_data* address_data) {
  //assert(!local_shadow_space[threadid].empty());
  //std::map<ADDRINT, struct local_access_hist>* thread_access_map = local_shadow_space[threadid].top();
  struct local_access_hist** thread_access_map = local_shadow_space[threadid].top();
  size_t local_prim_index = (addr >> 22) & 0x3ff;
  struct local_access_hist* local_primary_entry = thread_access_map[local_prim_index];
  if (local_primary_entry == NULL) {
    //std::cout << "local mmap" << std::endl;
    size_t local_sec_length = (SS_SEC_TABLE_ENTRIES) * sizeof(struct local_access_hist);
    local_primary_entry = (struct local_access_hist*)mmap(0, local_sec_length, PROT_READ| PROT_WRITE, MMAP_FLAGS, -1, 0);
    thread_access_map[local_prim_index] = local_primary_entry;
  }

  size_t local_offset = (addr) & 0x3fffff;
  struct local_access_hist* local_addr_vec = local_primary_entry + local_offset;
  //std::cout << "START " << local_addr_vec << std::endl;
  if (local_addr_vec->read_addr_data == NULL && local_addr_vec->write_addr_data == NULL) {
    //std::cout << "local vec null" << std::endl;
    //check if global shadow space has single read/write
    // if it does not add this access to global
    // if it does check if parallel
    size_t primary_index = (addr >> 22) & 0x3ff;
    struct global_access_hist* primary_entry = shadow_space[primary_index];
    if (primary_entry == NULL) {
      size_t sec_length = (SS_SEC_TABLE_ENTRIES) * sizeof(struct global_access_hist);
      primary_entry = (struct global_access_hist*)mmap(0, sec_length, PROT_READ| PROT_WRITE,
						  MMAP_FLAGS, -1, 0);
      shadow_space[primary_index] = primary_entry;
    }

    size_t offset = (addr) & 0x3fffff;
    struct global_access_hist* addr_vec = primary_entry + offset;
    if (addr_vec == NULL) {
      //std::cout << "global vec null" << std::endl;
      //struct global_access_hist g_access_hist;
      addr_vec->single_read = addr_vec->single_write = addr_vec->rr1 = addr_vec->rr2 = addr_vec->rw1 = 
	addr_vec->rw2 = addr_vec->wr1 = addr_vec->wr2 = addr_vec->ww1 = addr_vec->ww2 = NULL;
      ((accessType == READ) ? addr_vec->single_read:addr_vec->single_write) = address_data; 
      // insert no need to check
      //shadow_space.insert(std::pair<ADDRINT, struct global_access_hist>(addr, g_access_hist));
    } else {
      //struct global_access_hist g_access_hist = shadow_space.at(addr);
      if(accessType == READ) {
	if (addr_vec->single_read == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->single_read->task, threadid)) {
	  //update need check
	  addr_vec->single_read = address_data;
	}
	//struct global_access_hist g_access_hist = shadow_space.at(addr);
	if (check_single_read(address_data, *addr_vec, addr, threadid))
	  return;
      } else {
	if (addr_vec->single_write == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->single_write->task, threadid)) {
	  //update need check
	  addr_vec->single_write = address_data;
	} else if (addr_vec->single_write_1 == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->single_write_1->task, threadid)) {
	  //update need check
	  addr_vec->single_write_1 = address_data;
	}	
	
	//struct global_access_hist g_access_hist = shadow_space.at(addr);
	if (check_single_write(address_data, *addr_vec, addr, threadid))
	  return;
      }
    }

    //update local access history
    //struct local_access_hist access_hist;
    local_addr_vec->read_addr_data = local_addr_vec->write_addr_data = NULL;
    ((accessType == READ) ? local_addr_vec->read_addr_data:local_addr_vec->write_addr_data) = address_data;
    //thread_access_map->insert( std::pair<ADDRINT, struct local_access_hist>(addr, access_hist) );
  } else {
    //std::cout << "local not  null" << std::endl;
    size_t primary_index = (addr >> 22) & 0x3ff;
    struct global_access_hist* primary_entry = shadow_space[primary_index];
    assert (primary_entry != NULL);
    size_t offset = (addr) & 0x3fffff;
    struct global_access_hist* addr_vec = primary_entry + offset;
    assert (addr_vec != NULL);

    //assert(shadow_space.count(addr) != 0);
    if (accessType == READ) {
      //struct global_access_hist g_access_hist = shadow_space.at(addr);
      if (check_single_read(address_data, *addr_vec, addr, threadid))
	return;
      if(local_addr_vec->read_addr_data == NULL) {
	//update global
	if (addr_vec->single_read == NULL || 
	    !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->single_read->task, threadid)) {
	  addr_vec->single_read = address_data;
	}
	//update local
	local_addr_vec->read_addr_data = address_data;
      } else {
	// Check for RR access pattern
	if (Address_data::locksetEmpty(address_data->lockset, (local_addr_vec->read_addr_data)->lockset)) {
	  if (addr_vec->rr1 == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->rr1->task, threadid)) {
	    addr_vec->rr1 = local_addr_vec->read_addr_data;
	    addr_vec->rr2 = address_data;
	    //struct global_access_hist g_access_hist = shadow_space.at(addr);
	    if (check_rr(*addr_vec, addr, threadid))
	    return;
	  }
	}
      }
      // Check for WR access pattern
      if (local_addr_vec->write_addr_data && Address_data::locksetEmpty(address_data->lockset, (local_addr_vec->write_addr_data)->lockset)) {
	if (addr_vec->wr1 == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->wr1->task, threadid)) {
	  addr_vec->wr1 = local_addr_vec->write_addr_data;
	  addr_vec->wr2 = address_data;
	  //struct global_access_hist g_access_hist = shadow_space.at(addr);
	  if (check_wr(*addr_vec, addr, threadid))
	    return;
	}
      }

    } else {//accessType == WRITE

      if(local_addr_vec->write_addr_data == NULL) {
	//update global
	if (addr_vec->single_write == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->single_write->task, threadid)) {
	  addr_vec->single_write = address_data;
	} else if (addr_vec->single_write_1 == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->single_write_1->task, threadid)) {
	  addr_vec->single_write_1 = address_data;
	}

	//update local
	local_addr_vec->write_addr_data = address_data;
      } else {
	// Check for WW access pattern
	if (Address_data::locksetEmpty(address_data->lockset, (local_addr_vec->write_addr_data)->lockset)) {
	  if (addr_vec->ww1 == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->ww1->task, threadid)) {
	    addr_vec->ww1 = local_addr_vec->write_addr_data;
	    addr_vec->ww2 = address_data;
	    //struct global_access_hist g_access_hist = shadow_space.at(addr);
	    if (check_ww(*addr_vec, addr, threadid))
	      return;
	  }
	}
      }
      // Check for RW access pattern
      if (local_addr_vec->read_addr_data && Address_data::locksetEmpty(address_data->lockset, (local_addr_vec->read_addr_data)->lockset)) {
	if (addr_vec->rw1 == NULL || !taskGraph->areParallel(tidToTaskIdMap[threadid].top(), addr_vec->rw1->task, threadid)) {
	  addr_vec->rw1 = local_addr_vec->read_addr_data;
	  addr_vec->rw2 = address_data;
	  //struct global_access_hist g_access_hist = shadow_space.at(addr);
	  if (check_rw(*addr_vec, addr, threadid))
	    return;
	}
      }
      //struct global_access_hist g_access_hist = shadow_space.at(addr);
      if (check_single_write(address_data, *addr_vec, addr, threadid))
	return;

    }
  }
}

extern "C" void RecordMem(THREADID threadid, void * access_addr, AccessType accessType) {

  PIN_GetLock(&lock, 0);

  // addr for atomic sets
  ADDRINT addr;
  if (atomic_sets.count((ADDRINT) access_addr) == 0) {
    addr = (ADDRINT) access_addr;
  } else {
    addr = atomic_sets.at((ADDRINT) access_addr);
  }

  // Exceptions
  if(exceptions(threadid, addr)){ 
    PIN_ReleaseLock(&lock);
    return;
  }

  //form address_data, get lockset
  struct Lockset_data* curLockset = thd_lockset[threadid].top();
  struct Address_data* address_data = new Address_data(taskGraph->getCurTask(threadid), accessType);
  address_data->lockset = curLockset->createLockset();

  /////////////////////////////////////////////////////
  // check for access pattern and add to global if necessary
  update_access_histories(threadid, addr, accessType, address_data);

  /////////////////////////////////////////////////////

  PIN_ReleaseLock(&lock);
}

extern "C" void Fini()
{
  report.open("violations.out");

  for (std::map<ADDRINT,struct violation*>::iterator it=all_violations.begin();
       it!=all_violations.end(); ++it) {
    struct violation* viol = it->second;
    report_AV(it->first, viol->a1, viol->a2, viol->a3);
  }
  //std::cout << "Number of violations = " << all_violations.size() << std::endl;
  //std::cout << "Size of shadow space = " << shadow_space.size() <<
  //" max size = " << shadow_space.max_size() << std::endl;

  report.close();
}

#endif
