#include "PDR.hpp"
#include "BMC.hpp"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
using namespace std;
using namespace std::chrono;

int pdr_thread = 0, pdr_thread_count = 0, bmc_thread = 0, bmc_thread_count = 0;

void *thread_start_bmc(void *aiger){
    int nframes = INT_MAX;
    int thread_index = ++bmc_thread_count; //start from 1
    Aiger* aiger_data = ((Aiger *)aiger); 
    BMC bmc(aiger_data, 0, nframes, thread_index, bmc_thread);
    int res_bmc = bmc.check(); 
    if(!no_output) 
        cout << "res_bmc" + to_string(thread_index) +  "= " + to_string(res_bmc) + "\n";
    pthread_exit(NULL); 
}

void *thread_start_pdr(void *aiger){
    int thread_index = pdr_thread_count++; //start from 0
    Aiger* aiger_data = ((Aiger *)aiger);
    PDR pdr(aiger_data, thread_index, 1, 1, 1, 1);
    int res_pdr = pdr.check();   
    if(!no_output) 
        cout << "res_pdr" + to_string(thread_index) +  "= " + to_string(res_pdr) + "\n";
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
    auto t_begin = system_clock::now();
    if(!no_output) cout<<"c USAGE: ./modelchecker [<option>]* [propertyIndex] <aig-file>\n";
    Aiger *aiger = load_aiger_from_file(string(argv[argc-1]));
    int property_index = 0;
    bool pr = 0, acc = 0, sc = 0, addone = 0;
    for (int i = 1; i < argc-1; ++i){
        if (string(argv[i]) == "-pr")
            pr = 1;
        else if (string(argv[i]) == "-uc")  
            acc = 1;
        else if (string(argv[i]) == "-sc")  
            sc = 1;
        else if (string(argv[i]) == "-ao")  
            addone = 1;
        else if (string(argv[i]) == "-bmc"){
            i++;
            assert(i < argc-1);
            bmc_thread = (unsigned) atoi(argv[i]);
            assert(bmc_thread >=0);
            if(bmc_thread > 64) bmc_thread = 64;
        }
        else if (string(argv[i]) == "-pdr"){
            i++;
            assert(i < argc-1);
            pdr_thread = (unsigned) atoi(argv[i]);
            assert(pdr_thread >=0);
            if(pdr_thread > 4) pdr_thread = 4;
        }
        else 
            property_index = (unsigned) atoi(argv[i]);
    }
    // allow parameter input and execute single thread pdr only when pdr_thread==0 and bmc_thread==0
    if(pdr_thread == 0 and bmc_thread == 0){
        PDR pdr(aiger, -1, acc, pr, 1, 1);
        int res_pdr = pdr.check();  
    }
    else{
        pthread_t tpdr[8], tbmc[130];
        for(int t = 0; t < pdr_thread; t++){
            int ret = pthread_create (&tpdr[t], NULL, thread_start_pdr, (void *)aiger); 
        }
        for(int t = 0; t < bmc_thread; t++){
            int ret5 = pthread_create (&tbmc[t], NULL, thread_start_bmc, (void *)aiger); 
        }
        
        for(int t = 0; t < pdr_thread; t++)
            pthread_join(tpdr[t], NULL);
        for(int t = 0; t < bmc_thread; t++)
            pthread_join(tbmc[t], NULL);
    }

    if(RESULT  == 10 and !no_output)
        cout << 1 << endl;
    else if(RESULT  == 20)
        cout << 0 << endl;
    
    delete aiger;
    auto t_end = system_clock::now();
    auto duration = duration_cast<microseconds>(t_end - t_begin);
    double time_in_sec = double(duration.count()) * microseconds::period::num / microseconds::period::den;
    cout<<"c time = "<<time_in_sec<<endl;
    return 0;
}