clear;
clear;
#./rebuild.sh;
# cd /home/zhulf/portfolioMC
# cd /home/zhulf/project/Modelchecker-HWMCC24
make clean;
make -j;

#sat
#./modelchecker  /home/zhulf/modelchecker/testcase/bug_instance/prodcellp0neg.aag
./modelchecker  /home/zhulf/modelchecker/test4_fold.aag
#./modelchecker /home/zhulf/modelchecker/test4_2b.aag
#./modelchecker /home/zhulf/data/HWMCC24/trace.aag
#./modelchecker /home/zhulf/modelchecker/test4.aag


#unsat
#./modelchecker /home/zhulf/data/MC/hwmcc2015/data/bobsm5378d2.aig
#./modelchecker /home/zhulf/modelchecker/test4_revised.aag 
#./modelchecker /home/zhulf/data/HWMCC24/test4.aag