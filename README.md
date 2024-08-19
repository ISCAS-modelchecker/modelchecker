# portfolioMC
A parallel model checker compatible with BMC and PDR

## Usage
To build:

```
./rebulid.sh
make clean
make -j
```

To Run:

```
$ ./modelchecker <AIGER_file> 
```

## Result
We conducted experiments on the benchmarks from the HWMCC competitions in 2015, 2017, and 2020. In the table below, *4thread* denotes a 4-thread parallel portfolio PDR, *4thread_acc* indicates the use of assumption-core consistency heuristics in the *4thread* PDR method, and *igl* and *abc* are the two leading contemporary open-source model checkers.
|    HWMCC2015   |   SAT   |  UNSAT  |  ALL    |
| :------------: | :-----: | :-----: | :-----: |
|    4thread     | 74      |  199    |   273   |
|   4thread-acc  |   79    |    197  |   276   |
| 4thread-acc-debug |   79    |    197  |   276   |
|   IGOODLEMMA   | 76      |   203   |  279    |
|    ABC         | 68      |   198   |   266   |

|    HWMCC2017   |   SAT   |  UNSAT  |  ALL    |
| :------------: | :-----: | :-----: | :-----: |
|    4thread     | 38      |  88    |   126   |
|   4thread-acc  |   39    |    86  |   125   |
| 4thread-acc-debug |   38    |    87  |   125   |
|   IGOODLEMMA   | 37      |   87   |  124    |
|    ABC         | 39      |   91   |   130   |

|    HWMCC2020   |   SAT   |  UNSAT  |  ALL    |
| :------------: | :-----: | :-----: | :-----: |
|    4thread     | 30      |  202    |   232   |
|   4thread-acc  |   34    |    203  |   237   |
| 4thread-acc-debug |   39    |    204  |   243   |
|   IGOODLEMMA   | 33      |   195   |  228    |
|    ABC         | 27      |   191   |   218   |

