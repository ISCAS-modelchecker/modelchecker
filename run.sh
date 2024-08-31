clear;
clear;
# ./rebuild.sh;
make clean;
make -j;

./modelchecker  /home/zhulf/modelchecker/testcase/bug_instance/prodcellp0neg.aag
#./modelchecker  /home/zhulf/modelchecker/test4_fold.aag
#./modelchecker  /home/zhulf/modelchecker/test4.aag

#constraint 检验
#/home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/dspfilters_fastfir_second-p26.aig
#/home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/qspiflash_dualflexpress_divthree-p010.aig
#/home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/dspfilters_fastfir_second-p18.aig 关闭ctg还是1
#/home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/zipcpu-pfcache-p01.aig
# /home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/qspiflash_dualflexpress_divfive-p143.aig
# /home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/zipversa_composecrc_prf-p00.aig
#/home/zhulf/portfolioMC/modelchecker /home/zhulf/modelchecker/test4_revised.aag
#./modelchecker /home/zhulf/modelchecker/bug.aag
# /home/zhulf/portfolioMC/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/zipversa_composecrc_prf-p07.aig

#/home/zhulf/IC3ref/IC3 <  /home/zhulf/modelchecker/bug.aag
#/home/zhulf/aiger-1.9.1/aigtoaig /home/zhulf/data/MC/hwmcc2020/data/qspiflash_dualflexpress_divthree-p010.aig /home/zhulf/modelchecker/bug.aag
#/home/zhulf/aiger-1.9.1/aigsim -w /home/zhulf/modelchecker/bug.aag /home/zhulf/witness.txt