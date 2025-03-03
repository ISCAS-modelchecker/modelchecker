#include "PDR.hpp"
#include "BMC.hpp"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
using namespace std::chrono;

int pdr_thread = 0, current_pdr_execution = 0, bmc_thread = 0, current_bmc_execution = 0;

void *thread_start_bmc(void *aiger){
    int nframes = INT_MAX;
    int thread_index = ++current_bmc_execution; //start from 1
    Aiger* aiger_data = ((Aiger *)aiger); 
    BMC bmc(aiger_data, nframes, thread_index, bmc_thread);
    int res_bmc = bmc.check(); 
    if(moreinfo_for_bmc and res_bmc) 
        cout << "best configuration: bmc thread " + to_string(thread_index-1) + "\n";
    pthread_exit(NULL); 
}

void *thread_start_pdr(void *aiger){
    int thread_index = current_pdr_execution++; //start from 0
    Aiger* aiger_data = ((Aiger *)aiger);
    PDR pdr(aiger_data, thread_index, 1, 1, 1);
    int res_pdr = pdr.check();   
    if(moreinfo_for_pdr and res_pdr) 
        cout << "best configuration: pdr thread " + to_string(thread_index) + "\n";
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
    auto t_begin = system_clock::now();
    if (argc == 2 && std::string(argv[1]) == "-h") {
        std::cout << "USAGE: ./modelchecker [OPTIONS] <aig-file>\n\n"
                     "OPTIONS:\n"
                     "  -bmc <bmc_thread>        Set the number of BMC threads (0-12). Default: 0\n"
                     "  -pdr <pdr_thread>        Set the number of PDR threads (0-4). Default: 0\n"
                     "  -pindex <property_index> Specify the property index for verification. Default: 0\n"
                     "  -witness                 Output witness if a bug is found. Default: off\n"
                     "  -certificate             Output certificate if the circuit is safe. Default: off\n"
                     "  -uc                      Enable CPO-Driven UNSAT Core Generation (single-threaded PDR only)\n"
                     "  -pr                      Enable CPO-Driven Proof Obligation Propagation (single-threaded PDR only)\n"
                     "  -h                       Show this help message and exit.\n\n"
                     "Example:\n"
                     "  ./modelchecker -bmc 4 -pdr 2 -pindex 3 -witness example.aig\n";
        return 0;
    }

    cout << "USAGE: ./modelchecker [OPTIONS] <aig-file>\n";
    cout << "Try './modelchecker -h' for more details.\n";
    int property_index = 0;
    bool pr = 0, uc = 0, witness = 0, certificate = 0;
    for (int i = 1; i < argc-1; ++i){
        string arg = argv[i];
        if (arg == "-pr")
            pr = 1;
        else if (arg == "-uc")  
            uc = 1;
        else if (arg == "-witness")
            witness = 1;
        else if (arg == "-certificate")
            certificate = 1;
        else if (arg == "-bmc" || arg == "-pdr" || arg == "-pindex"){
            if (i + 1 >= argc) {
                cerr << "Error: Missing value for " << arg << ". Defaulting to 0.\n";
                continue;
            }
            string value = argv[++i];
            if (arg == "-bmc")
                bmc_thread = validateInput(arg, value, 0, 12, 0);
            else if (arg == "-pdr")
                pdr_thread = validateInput(arg, value, 0, 4, 0);
            else if (arg == "-pindex")
                property_index = validateInput(arg, value, 0, INT_MAX, 0);
        }
    }
    Aiger *aiger = load_aiger_from_file(string(argv[argc-1]), property_index, witness, certificate);
    aiger->translate_to_dimacs();

    if(pdr_thread == 0 and bmc_thread == 0){
        cerr << "Warning: Both -bmc and -pdr thread counts are set to 0. Defaulting to single-threaded PDR." << endl;
        PDR pdr(aiger, -1, uc, pr, 1);
        int res_pdr = pdr.check();  
    }
    else{
        pthread_t tpdr[8], tbmc[16];
        for(int t = 0; t < pdr_thread; t++){
            int ret_pdr = pthread_create (&tpdr[t], NULL, thread_start_pdr, (void *)aiger); 
        }
        for(int t = 0; t < bmc_thread; t++){
            int ret_bmc = pthread_create (&tbmc[t], NULL, thread_start_bmc, (void *)aiger); 
        }
        
        for(int t = 0; t < pdr_thread; t++)
            pthread_join(tpdr[t], NULL);
        for(int t = 0; t < bmc_thread; t++)
            pthread_join(tbmc[t], NULL);
    }

    if(RESULT  == 10)
        cout << "result: unsafe\n";
    else if(RESULT  == 20)
        cout << "result: safe\n";
    
    delete aiger;
    auto t_end = system_clock::now();
    auto duration = duration_cast<microseconds>(t_end - t_begin);
    double time_in_sec = double(duration.count()) * microseconds::period::num / microseconds::period::den;
    cout << "time = "<< time_in_sec << "s\n";
    return 0;
}