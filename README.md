# modelchecker
A bit-level parallel model checking solver supporting up to 16 threads, compatible with both BMC and PDR.

## Usage

### **Building Modelchecker**
Run the following commands in the project directory:
```sh
./rebuild.sh
make clean
make -j
```


### **Running Modelchecker**
```sh
./modelchecker [OPTIONS] <AIGER_file>
```


#### **Options:**
- `-bmc <bmc_thread>`  — Set the number of BMC threads (0-12). Default: 0  
- `-pdr <pdr_thread>`  — Set the number of PDR threads (0-4). Default: 0  
- `-pindex <property_index>`  — Specify the property index for verification. Default: 0  
- `-witness`  — Output witness if a bug is found. Default: off  
- `-certificate`  — Output certificate if the circuit is safe. Default: off  
- `-uc`  — Enable CPO-Driven UNSAT Core Generation (only for single-threaded PDR)  
- `-pr`  — Enable CPO-Driven Proof Obligation Propagation (only for single-threaded PDR)  
- `-h`  — Show help message and exit  


### **Example Usage**
```sh
./modelchecker -bmc 4 -pdr 2 -pindex 3 -witness example.aig
```
This runs Modelchecker with 4 BMC threads, 2 PDR threads, and checks property index 3 while enabling witness output.


## **Output Explanation**
- **For unsafe circuits**, Modelchecker prints `result: unsafe` along with runtime information. If `-witness` is enabled, it also outputs a witness.  
- **For safe circuits**, Modelchecker prints `result: safe` along with runtime details.  
  - If `-certificate` is enabled, the circuit's proof certificate will be generated in both AIG and AAG formats as `certificate.aig` and `certificate.aag` in the working directory.

