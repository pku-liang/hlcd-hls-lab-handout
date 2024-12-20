CC = /usr/bin/gcc
CXX = /usr/bin/g++
TEST ?= 1
CFLAGS = -I$(shell pwd)/minisat/ -fpermissive -fPIC -std=c++17 -g

verifier: common.cpp verifier.cpp
	$(CXX) $(CFLAGS) -o $@ $^ $(CFLAGS)

scheduler.o: scheduler.cpp
	$(CXX) $(CFLAGS) -c -o $@ $^

sched: scheduler.o
	$(CXX) $(CFLAGS) main.cpp common.cpp scheduler.o libminisat.a -o $@

test: sched verifier
	./sched test/$(TEST)/ir.txt test/$(TEST)/op.txt schedule.txt
	./verifier test/$(TEST)/ir.txt test/$(TEST)/op.txt schedule.txt

test_minisat: test_minisat.cpp
	$(CXX) $(CFLAGS) test_minisat.cpp libminisat.a -o $@
	./$@

clean:
	rm -f *.o
	rm -f sched

.PHONY: test test_minisat