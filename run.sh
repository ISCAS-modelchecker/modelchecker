clear;
clear;
#./rebuild.sh;
make clean;
make -j;

#sat
#./modelchecker  -bmc 1 /home/zhulf/data/testcase/test4_fold.aag
#./modelchecker  /home/zhulf/data/testcase/test4_fold.aag
./modelchecker  -pdr 1 -bmc 1 /home/zhulf/data/testcase/bug_instance/prodcellp0neg.aag


#unsat
#./modelchecker /home/zhulf/data/MC/hwmcc2015/data/bobsm5378d2.aig
#./modelchecker /home/zhulf/modelchecker/test4_revised.aag 
#./modelchecker /home/zhulf/data/HWMCC24/test4.aag