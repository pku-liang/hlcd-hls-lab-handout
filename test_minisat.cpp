#include <minisat/core/Solver.h>

int main() {
  Minisat::Solver solver;
  
  Minisat::Var a, b, c;
  // 声明新变量
  a = solver.newVar();
  b = solver.newVar();
  c = solver.newVar();

  // 添加clause: a OR !b
  solver.addClause(Minisat::mkLit(a), Minisat::mkLit(b, true /*表示!b*/));

  // 添加可变长度的clause
  Minisat::vec<Minisat::Lit> clause;
  clause.push(Minisat::mkLit(a, true));
  clause.push(Minisat::mkLit(b, true));
  clause.push(Minisat::mkLit(c));
  solver.addClause(clause);

  bool result = solver.solve();
  if (result == true) {
    // true表示有解， false表示没有
    // 通过常量表示true，false
    printf("%d %d\n", Minisat::l_True, Minisat::l_False);

    // 查询solution中Literal对应的值
    printf("%d\n", solver.modelValue(Minisat::mkLit(a)));
    printf("%d\n", solver.modelValue(Minisat::mkLit(b)));
    // 这里查询的是!c对应的值
    printf("%d\n", solver.modelValue(Minisat::mkLit(c, true)));
  }
}