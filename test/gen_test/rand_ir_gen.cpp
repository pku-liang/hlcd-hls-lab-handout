#include <cstdio>
#include <random>
#include <vector>
#include <string>
using namespace std;

struct Op {
	string name;
	int n_operands;
	double delay;
	int latency;
	int limit;
};

int main(int argc, char **argv) {
	int seed = atoi(argv[1]);
	int n_mem = atoi(argv[2]);
	int n_in = atoi(argv[3]);
	int n_ir = atoi(argv[4]);
	int range = atoi(argv[5]);
	FILE *op_file = fopen(argv[6], "r");
	
	srand(seed);
	printf("%d %d %d\n", n_mem, n_in, n_ir);
	
	int n_op;
	double clock_period;
	
	fscanf(op_file, "%d%lf", &n_op, &clock_period);
	vector<Op> ops;
	for (int i = 0; i < n_op; ++i) {
		char name[128];
		int n_operands;
		double delay;
		int latency;
		int limit;

		fscanf(op_file, "%s%d%lf%d%d", name, &n_operands, &delay,
																	 &latency, &limit);

		ops.push_back({string(name), n_operands, delay, latency, limit});
	}

	for (int i = 0; i < n_ir; ++i) {
		int op_id = rand() % n_op;
		vector<int> operands;

		int n_operands = ops[op_id].n_operands;
		if (ops[op_id].name == "load" || ops[op_id].name == "store") {
			operands.push_back(rand() % n_mem);
			n_operands -= 1;
		}
		for (int j = 0; j < n_operands; ++j) {
			int limit = n_in + min(i, range);
			int rnd_idx = rand() % limit;
			
			if (rnd_idx < n_in)
				operands.push_back(n_mem + rnd_idx + 1);
			else
				operands.push_back(n_mem + n_in + i + 1 - limit + rnd_idx);
		}
		printf("%s", ops[op_id].name.c_str());
		for (int in : operands)
			printf(" %d", in);
		printf("\n");
	}	
}
