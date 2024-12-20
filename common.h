#pragma once

#include <vector>
#include <string>

class DFG;
class Stmt;
class Op;

struct Op {
  int latency;
  int limit;
  int idx;
  double delay;
  std::string name;
};

struct Stmt {
  Op *op;
  int idx;
  int start_cycle;
  std::vector<Stmt*> ins;

  virtual bool is_mem_stmt() { return false; }
  virtual int get_arr_idx() { return -1; }
};

struct MemStmt : Stmt {
  int arr_idx;

  virtual bool is_mem_stmt() override { return true; }
  virtual int get_arr_idx() override { return arr_idx; }
};

struct DFG {
  int num_memory;
  std::vector<Stmt*> stmts;
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
