#include <cstdio>
#include <string>
#include <unordered_map>
#include <cassert>
#include "common.h"
using namespace std;

void parse(FILE *dfg_file, FILE *op_file, DFG *dfg, vector<Op*> &ops, double &clock_period) {
  int n_mem, n_input, n_stmts;
  int n_ops;

  fscanf(dfg_file, "%d%d%d", &n_mem, &n_input, &n_stmts);
  fscanf(op_file, "%d%lf", &n_ops, &clock_period);

  ops.resize(n_ops);
  unordered_map<string, tuple<Op*, int>> op_mapping;
  
  for (int i = 0; i < n_ops; ++i) {
    char name[128];
    fscanf(op_file, "%s", name);

    string name_str(name);
    int has_result, n_operand, latency, limit;
    double delay;

    fscanf(op_file, "%d%lf%d%d", &n_operand, &delay, &latency, &limit);
    ops[i] = new Op{latency, limit, i, delay, name_str};
    op_mapping[name_str] = {ops[i], n_operand};
  }

  dfg->num_memory = n_mem;

  for (int i = 0; i < n_stmts; ++i) {
    char name[128];
    fscanf(dfg_file, "%s", name);
    string name_str(name);

    assert(op_mapping.find(name_str) != op_mapping.end());

    int n_operands = get<1>(op_mapping[name_str]);
    Op *op = get<0>(op_mapping[name_str]);

    vector<int> operands;
    for (int j = 0; j < n_operands; ++j) {
      int opr;
      fscanf(dfg_file, "%d",&opr);
      if (opr == -1 || (n_mem < opr && opr <= n_input+n_mem))
        continue;
      // In the data structure, we number variables starting from 0
      if (opr > n_input+n_mem)
        operands.push_back(opr - n_input - n_mem - 1);
      else
        operands.push_back(opr - 1);
    }

    if (name_str == "load" || name_str == "store") {
      dfg->stmts.push_back(new MemStmt);
      ((MemStmt*)dfg->stmts.back())->arr_idx = operands[0];
      operands.erase(operands.begin());
    } else {
      dfg->stmts.push_back(new Stmt);
    }
    
    vector<Stmt*> ins;
    for (int i : operands) {
      ins.push_back(dfg->stmts[i]);
    }

    dfg->stmts.back()->ins = std::move(ins);
    dfg->stmts.back()->idx = i;
    dfg->stmts.back()->op = op;
    dfg->stmts.back()->start_cycle = -1;
  }
}

void get_deps_and_uses(DFG *dfg, vec2d<int> &deps, vec2d<int> &uses) {
  int n = dfg->stmts.size();
  deps = vec2d<int>(n);
  uses = vec2d<int>(n);

  for (int i = 0; i < n; ++i) {
    for (auto in : dfg->stmts[i]->ins) {
      uses[in->idx].push_back(i);
      deps[i].push_back(in->idx);
    }
  }

  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j) {
      if (!dfg->stmts[i]->is_mem_stmt() ||
          !dfg->stmts[j]->is_mem_stmt())
        continue;
      if (dfg->stmts[i]->get_arr_idx() !=
          dfg->stmts[j]->get_arr_idx())
        continue;

      if (dfg->stmts[i]->op->name == "load" &&
          dfg->stmts[j]->op->name == "load")
        continue;

      uses[i].push_back(j);
      deps[j].push_back(i);
    }
}