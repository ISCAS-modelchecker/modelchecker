# 默认目标
all: modelchecker

# modelchecker 目标的依赖和构建规则
modelchecker: BMC.hpp PDR.hpp aig.hpp basic.hpp sat_solver.hpp ipasir.h libcadical.a minisat/build/dynamic/lib/libminisat.so
	g++ -std=c++0x -O3 -o modelchecker BMC.cpp PDR.cpp aig.cpp main.cpp -g \
		-L. -lcadical \
		-Iminisat -Iminisat/minisat/simp -Iminisat/minisat/core -Iminisat/minisat/mtl \
		minisat/build/release/lib/libminisat.a \
		-lpthread

# 编译 fold.c 成对象文件 fold.o
#fold.o: fold.c fold.h
#	gcc -std=c11 -c fold.c -o fold.o

# 清理构建生成的文件
clean:
	rm -f modelchecker fold.o
