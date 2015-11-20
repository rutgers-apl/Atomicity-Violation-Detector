#ifndef OAFTASK_GRAPH_H
#define OAFTASK_GRAPH_H

#include <assert.h>

#define MMAP_FLAGS (MAP_PRIVATE| MAP_ANONYMOUS| MAP_NORESERVE)
#define NUM_GRAPH_NODES 65536


enum NodeType {
  NO_TYPE = 0,
  ASYNC = 1,
  FINISH = 2,
  STEP = 3
};


/* Invariants:
   index 0 --> dummy node 
   the graph starts from index 1 
*/

/* struct OAFTask:
   cur_step: index to the current step node
   parent: index to the current parent
   task_id: task id
   depth: height of the node from the root 
   seq_num: number identifying the siblings
   young_ns_child: Type of the node next to the step node
*/

struct OAFTask {

  size_t cur_step;
  size_t parent;
  size_t task_id;
  NodeType type;

  size_t depth;
  size_t seq_num;
  size_t num_children;  
  NodeType young_ns_child;
  
};

void initialize_oftask (size_t index,
			NodeType node_type, 
			size_t val){

  tgraph_nodes[index].cur_step = 0;
  tgraph_nodes[index].parent = 0;
  tgraph_nodes[index].task_id = val;
  tgraph_nodes[index].type = node_type;

  tgraph_nodes[index].depth = 0;
  tgraph_nodes[index].seq_num = 0;
  tgraph_nodes[index].num_children = 0;
  tgraph_nodes[index].young_ns_child = NO_TYPE;


}



struct oaftask* tgraph_nodes = NULL;
size_t last_allocated_node = 0;
std::stack<size_t> tgraph_cur[NUM_THREADS];

std::map<size_t, size_t> temp_cur_map;

void initialize_graph(){

  size_t length = NUM_GRAPH_NODES * sizeof(struct oaftask);
  tgraph_nodes = (struct oaftask*) mmap(0, length, PROT_READ|PROT_WRITE, MMAP_FLAGS, -1, 0);
  assert (tgraph_nodes != (void*)-1);
  size_t head_node = create_node(FINISH, 0);
  cur[0].push(head_node);
}

size_t create_node(NodeType node_type, size_t task_id){

  int index = ++last_allocated_node;
  assert(last_allocated_node < NUM_GRAPH_NODES);
  initialize_node(index, node_type, task_id); 

  return index;
}


bool task_graph_parallel(size_t cur_task, size_t remote_task);
size_t traverse_up (size_t cur_task, size_t depth);



