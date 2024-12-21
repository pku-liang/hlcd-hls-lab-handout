#include "common.h"
#include <algorithm>
#include <tuple>
#include <queue>
#include <cassert>
using namespace std;

void verify_deps_failed(DFG *dfg, int pred, int succ, int earliest_cycle) {
  printf("================\n");
  printf("||   FAILED   ||\n");
  printf("================\n");

  printf("Stmt %d depends on Stmt %d\n", succ+1, pred+1);
  printf("Stmt %d start at cycle %d\n", pred+1, dfg->stmts[pred]->start_cycle);
  printf("Stmt %d start at cycle %d, ", succ+1, dfg->stmts[succ]->start_cycle);
  printf("should be no earlier than %d\n", earliest_cycle);
  exit(1);
}

void verify_deps(DFG *dfg, const vec2d<int> &uses) {
  printf("Verifying Dependence....\n");
  int n_stmts = dfg->stmts.size();
  for (int i = 0; i < n_stmts; ++i) {
    for (int use : uses[i]) {
      int start_i = dfg->stmts[i]->start_cycle;
      int start_use = dfg->stmts[use]->start_cycle;
      int use_is_sequential = dfg->stmts[use]->op->latency > 0;

      // The latency cycle can be chained with other combinational ops
      int result_cycle = start_i + dfg->stmts[i]->op->latency - 1;
      if (result_cycle + use_is_sequential > start_use)
        verify_deps_failed(dfg, i, use, result_cycle + use_is_sequential);
    }
  }
}

void verify_clock_period_failed(double clock_period, DFG *dfg, const vector<int> &critical) {
  printf("================\n");
  printf("||   FAILED   ||\n");
  printf("================\n");

  printf("Target clock period is %.2lf ns\n", clock_period);
  printf("Critical path: ");
  for (auto id : critical) 
    printf("%d(%.2lf ns) ", id+1, dfg->stmts[id]->op->delay);
  printf("\n");
  exit(1);
}

void dfs_cp(DFG *dfg, const vec2d<int> &uses, int x, int in_cycle,
            double delay, double target, vector<int> &critical) {
  critical.push_back(x);
  delay += dfg->stmts[x]->op->delay;
  if (delay > target)
    verify_clock_period_failed(target, dfg, critical);
  
  for (int use : uses[x])
    if (dfg->stmts[use]->start_cycle == in_cycle)
      dfs_cp(dfg, uses, use, in_cycle, delay, target, critical);
  
  critical.pop_back();
}

void verify_clock_period(DFG *dfg, const vec2d<int> &uses, double clock_period) {
  printf("Verifying Clock Period....\n");
  int n_stmts = dfg->stmts.size();
  for (int i = 0; i < n_stmts; ++i) {
    vector<int> critical;
    int in_cycle = dfg->stmts[i]->start_cycle + dfg->stmts[i]->op->latency - 1;
    dfs_cp(dfg, uses, i, in_cycle, 0.0, clock_period, critical);
  }
}

void verify_resource_failed(const string &name, int limit, vector<int> ids, int cycle) {
  printf("================\n");
  printf("||   FAILED   ||\n");
  printf("================\n");

  printf("Usage of %s exceeds limit %d in cycle %d\n", name.c_str(), limit, cycle);
  printf("Stmts are: ");
  for (int id : ids)
    printf("%d ", id+1);
  printf("\n");
  exit(1);
}

void check_intervals(const string &name, vector<tuple<int, int, int>> &intv, int limit) {
  sort(intv.begin(), intv.end());
  queue<tuple<int, int, int>> active_stmts;

  for (auto [l, r, id] : intv) {
    while (!active_stmts.empty() && get<1>(active_stmts.front()) < l)
      active_stmts.pop();
    if (active_stmts.size() == limit) {
      vector<int> ids;
      while (!active_stmts.empty()) {
        ids.push_back(get<2>(active_stmts.front()));
        active_stmts.pop();
      }
      ids.push_back(id);
      verify_resource_failed(name, limit, ids, l);
    }
    active_stmts.push({l, r, id});
  }
}

void verify_resource(DFG *dfg, const vector<Op*> &ops) {
  printf("Verifying Resource Utilization....\n");
  int n_stmts = dfg->stmts.size();
  int n_ops = ops.size();

  for (int i = 0; i < n_ops; ++i) {
    if (ops[i]->limit == -1)
      continue;
    if (ops[i]->name == "load" || ops[i]->name == "store")
      continue;

    // (left, right, id)
    vector<tuple<int, int, int>> intv;

    for (int j = 0; j < n_stmts; ++j)
      if (dfg->stmts[j]->op == ops[i]) {
        int start = dfg->stmts[j]->start_cycle;
        int end = start + max(dfg->stmts[j]->op->latency, 1) - 1;
        intv.push_back({start, end, j});
      }

    check_intervals(ops[i]->name, intv, ops[i]->limit);
  }

  int memport_lim = 0;
  for (int i = 0; i < n_ops; ++i)
    if (ops[i]->name == "load" || ops[i]->name == "store") {
      memport_lim = ops[i]->limit;
      break;
    }

  int n_mems = dfg->num_memory;
  for (int i = 0; i < n_mems; ++i) {
    vector<tuple<int, int, int>> intv;
    if (dfg->stmts[i]->is_mem_stmt() && 
        dfg->stmts[i]->get_arr_idx() == n_mems) {
      int start = dfg->stmts[i]->start_cycle;
      int end = start + max(dfg->stmts[i]->op->latency, 1) - 1;
      intv.push_back({start, end, i});
    }

    check_intervals("load/store", intv, memport_lim);
  }
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: %s path/to/dfg path/to/op [path/to/schedule]\n", argv[0]);
    return -1;
  }

  DFG dfg;
  vector<Op*> ops;
  double clock_period;

  FILE *dfg_file = fopen(argv[1], "r");
  FILE *op_file = fopen(argv[2], "r");

  FILE *sched_file;
  if (argc >= 4)
    sched_file = fopen(argv[3], "r");
  else
    sched_file = stdin;

  parse(dfg_file, op_file, &dfg, ops, clock_period);
  
  int sat_sdc_lat, asap_lat;
  if (fscanf(dfg_file, "%d%d", &sat_sdc_lat, &asap_lat) != 2) {
    sat_sdc_lat = -1;
    asap_lat = -1;
  }

  int n_stmts = dfg.stmts.size();
  
  for (int i = 0; i < n_stmts; ++i) {
    int start_cycle;
    fscanf(sched_file, "%d", &start_cycle);
    assert(start_cycle > 0);
    dfg.stmts[i]->start_cycle = start_cycle;
  }

  vec2d<int> deps, uses;
  get_deps_and_uses(&dfg, deps, uses);

  verify_deps(&dfg, uses);
  verify_clock_period(&dfg, uses, clock_period);
  verify_resource(&dfg, ops);

  int total_latency = 0;
  for (int i = 0; i < n_stmts; ++i) {
    total_latency = max(dfg.stmts[i]->start_cycle + max(dfg.stmts[i]->op->latency, 1)-1, 
                        total_latency);
  }

  printf("================\n");
  printf("||    PASS    ||\n");
  printf("================\n");

  printf("Total latency: %d\n", total_latency);

  if (sat_sdc_lat > 0) {
    double ratio = 20.0 / (double)(sat_sdc_lat - asap_lat);
    printf("Score: %.2lf\n", min(100.0, max(60.0, (double)(total_latency-asap_lat) * ratio + 80.0)));
  }

  return 0;
}