#include "PDR.hpp"
#include "BMC.hpp"
#include "BMPDR.hpp"
#include "aig.hpp"
#include "basic.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
using namespace std;
using namespace std::chrono;

void *thread_start_bmc(void *aiger){
    int property_index = 0;
    int nframes = INT_MAX;
    Aiger* aiger_data = ((Aiger *)aiger);
    
    BMC bmc(aiger_data, property_index, nframes);
    bmc.initialize();
    int res_bmc = bmc.check(); 
    cout << "res_bmc = " << res_bmc << endl;
    pthread_exit(NULL); 
}

void *thread_start_pdr(void *aiger){
    int property_index = 0;
    Aiger* aiger_data = ((Aiger *)aiger);
    
    PDR pdr(aiger_data, property_index);
    int res_pdr = pdr.check();   
    cout << "res_pdr = " << res_pdr << endl;
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
    auto t_begin = system_clock::now();
    cout<<"c USAGE: ./modelchecker <aig-file> [propertyIndex]"<<endl;
    Aiger *aiger = load_aiger_from_file(string(argv[1]));
    int property_index = 0;
    if(argc > 2){
        property_index = (unsigned) atoi(argv[2]);
    }
         
    pthread_t tbmc, tpdr;
    int ret = pthread_create (&tbmc, NULL, thread_start_bmc, (void *)aiger);  //int ret = pthread_create (&tbmc, NULL, thread_start_bmc, (void *)aiger);  
    int ret2 = pthread_create (&tpdr, NULL, thread_start_pdr, (void *)aiger);  //int ret2 = pthread_create (&tpdr, NULL, thread_start_pdr, (void *)aiger);  
    pthread_join(tbmc, NULL);
    pthread_join(tpdr, NULL);
    if(PEBMC_result  == 10)
        cout << 1 << endl;
    else if(PEBMC_result  == 20)
        cout << 0 << endl;
    //sleep(10);
    
    delete aiger;
    auto t_end = system_clock::now();
    auto duration = duration_cast<microseconds>(t_end - t_begin);
    double time_in_sec = double(duration.count()) * microseconds::period::num / microseconds::period::den;
    cout<<"c time = "<<time_in_sec<<endl;
    return 1;
}