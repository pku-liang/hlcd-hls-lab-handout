#include <cstdio>
#include <string>
#include <cstring>
#include "common.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s path/to/dfg path/to/ops [path/to/schedule]", argv[0]);
    exit(1);
  }

  FILE *dfg_file = fopen(argv[1], "r");
  FILE *op_file = fopen(argv[2], "r");

  FILE *schedule_file;
  if (argc >= 4)
    schedule_file = fopen(argv[3], "w");
  else
    schedule_file = stdout;
  
  fflush(stdout);
  DFG dfg;
  std::vector<Op*> ops;
  double clock_period;

  parse(dfg_file, op_file, &dfg, ops, clock_period);
  
  schedule(&dfg, ops, clock_period);

  for (Stmt *stmt : dfg.stmts) {
    fprintf(schedule_file, "%d\n", stmt->start_cycle);
  }
}
