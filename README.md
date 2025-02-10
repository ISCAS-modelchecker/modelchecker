# modelchecker
A 16-thread bit-level parallel MC solver, compatible with BMC and PDR

## Usage
To build Modelchecker, run the following command in the project directory:

```
./rebulid.sh
make clean
make -j
```

To use Modelchecker, run the following command in the project directory:

```
$ ./modelchecker [-pdr <pdr_thread>] [-bmc <bmc_thread>] <AIGER_file> 
```

Note that the maximum number of threads for PDR is 4, while the maximum number of threads for BMC is 12.

For **unsafe** circuits, Modelchecker will output a a witness to the standard output and display the program's runtime. 

For **safe** circuits, Modelchecker will output "0" along with the runtime. Additionally, it will generate the circuit's certificate in both AIG and AAG formats in the current working directory, named `certificate.aig` and `certificate.aag` respectively.


<!-- ## Result
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
|    ABC         | 27      |   191   |   218   | -->

