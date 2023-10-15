clear;
clear;
# ./rebuild.sh;
make clean;
make -j;

#/home/zhulf/modelchecker/modelchecker /home/zhulf/modelchecker/test4.aig
/home/zhulf/PEBMC/modelchecker /home/zhulf/modelchecker/test4_fold.aig

#/home/zhulf/modelchecker/modelchecker  /home/zhulf/modelchecker/testcase/bug_instance/prodcellp0neg.aag
#/home/zhulf/modelchecker/modelchecker  /home/zhulf/data/MC/hwmcc2020/data/dspfilters_fastfir_second-p05.aig
#time /home/zhulf/modelchecker/modelchecker /home/zhulf/data/MC/hwmcc2020-abc/data/zipcpu-busdelay-p36.aig   # mic(cube)特别长 每次drop一个literal很慢
#time /home/zhulf/modelchecker/modelchecker /home/zhulf/data/MC/hwmcc2020-abc/data/frogs.5.prop1-func-interl.aig
#time /home/zhulf/modelchecker/modelchecker /home/zhulf/data/MC/hwmcc2020-abc/data/zipcpu-zipmmu-p39.aig

#time ./modelchecker /home/zhulf/data/MC/hwmcc2020-abc/data/blocks.4.prop1-back-serstep.aig        m0 55.8 其他无解
#time ./modelchecker /home/zhulf/data/MC/hwmcc2020-abc/data/qspiflash_dualflexpress_divfive-p009.aig m0 m1 20s m7 m8 无解
#time ./modelchecker /home/zhulf/data/MC/hwmcc2020-abc/data/vis_arrays_buf_bug.aig #其他求解器都秒解 我们这个很慢

#time ./modelchecker ./testcase/bug_instance/bc57sensorsp1.aig  #40s
#time ./modelchecker ./testcase/bug_instance/anderson.3.prop1-back-serstep.aig #秒出
#time ./modelchecker ./testcase/zipversa_composecrc_prf-p03.aig #在level7死循环
#time ./modelchecker /home/zhulf/data/MC/hwmcc2020/data/zipversa_composecrc_prf-p22.aig 不化简无法读取




