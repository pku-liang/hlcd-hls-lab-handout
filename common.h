#pragma once

#include <vector>
#include <string>

class DFG;
class Stmt;
class Op;

struct Op {
  int latency;       // latency of this op. 0 means this op is combinational
  int limit;         // resource limit of this op. -1 means unlimited
  int idx;           // index of this op in the `ops` vector
  double delay;      // delay of this op
  std::string name;  // name of this op
};

struct Stmt {
  Op *op;                   // pointer to the related op
  int idx;                  // index of this stmt in the `stmts` vector
  int start_cycle;          // start cycle of this op, you need to fill this member
  std::vector<Stmt*> ins;   // inputs of this op. constant and external inputs are ignored

  virtual bool is_mem_stmt() { return false; }
  virtual int get_arr_idx() { return -1; }
};

struct MemStmt : Stmt {
  int arr_idx;               // index of the accessed memory

  virtual bool is_mem_stmt() override { return true; }
  virtual int get_arr_idx() override { return arr_idx; }
};

struct DFG {
  int num_memory;           // number of memories
  std::vector<Stmt*> stmts; // all stmts read from the DFG file
  ~DFG() {
    for (auto stmt_ptr : stmts)
      delete stmt_ptr;
  }
};

/// vector of vectors
template<typename T>
using vec2d = std::vector<std::vector<T>>;

/* Get the dep and use relationship from the dfg, including load and store.
 * deps[i] stores the indices of stmts that i depends on 
 * uses[i] stores the indices of stmts that depends on i
 * j \in deps[i] <-> i \in uses[j]
*/
void get_deps_and_uses(DFG *dfg, vec2d<int> &deps, vec2d<int> &uses);

void schedule(DFG *dfg, const std::vector<Op*> &ops, double clock_period);

// common parser function
void parse(FILE *dfg_file, FILE *op_file, DFG *dfg, std::vector<Op*> &ops, double &clock_period);
