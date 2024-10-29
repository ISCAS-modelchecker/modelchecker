#include "PDR.hpp"
#include "BMC.hpp"
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

int thread_count = 0, bmc_thread = 0, bmc_thread_count = 0;
bool sc = 0, acc = 0;

void *thread_start_bmc(void *aiger){
    int nframes = INT_MAX;
    int thread_index = ++bmc_thread_count; //start from 1
    Aiger* aiger_data = ((Aiger *)aiger); 
    BMC bmc(aiger_data, 0, nframes, thread_index, bmc_thread);
    int res_bmc = bmc.check(); 
    if(!no_output) cout << "res_bmc" + to_string(thread_index) +  "= " + to_string(res_bmc) + "\n";
    pthread_exit(NULL); 
}

void *thread_start_pdr(void *aiger){
    int thread_index = thread_count++; //start from 0
    Aiger* aiger_data = ((Aiger *)aiger);
    PDR pdr(aiger_data, thread_index, 1, 1);
    int res_pdr = pdr.check();   
    if(!no_output) cout << "res_pdr" + to_string(thread_index) +  "= " + to_string(res_pdr) + "\n";
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
    //srand((unsigned int)time(NULL));
    //freopen("freopen.out","w",stdout);
    auto t_begin = system_clock::now();
    if(!no_output) cout<<"c USAGE: ./modelchecker <aig-file> [propertyIndex] [<option>]*\n";
    Aiger *aiger = load_aiger_from_file(string(argv[argc-1]));
    int property_index = 0;
    for (int i = 1; i < argc-1; ++i){
        if (string(argv[i]) == "-sc")
            sc = 1;
        else if (string(argv[i]) == "-acc")  
            acc = 1;
        else if (string(argv[i]) == "-bmc"){
            i++;
            assert(i < argc-1);
            bmc_thread = (unsigned) atoi(argv[i]);
            if(bmc_thread < 0 || bmc_thread > 12) bmc_thread = 0;
        }
        else 
            property_index = (unsigned) atoi(argv[i]);
    }
    //仅在执行单线程PDR时，允许命令行设置参数sc aac（PDR线程数仅能通过basic.hpp修改）
    //仅在命令行输入指令-bmc [int]时执行bmc 其中int为线程数量0-12
    if(thread_pdr and thread_4 == 0 and bmc_thread == 0){
        PDR pdr(aiger, -1, acc, sc);
        int res_pdr = pdr.check();  
    }
    else{
        pthread_t tpdr1, tpdr2, tpdr3, tpdr4, tbmc[128];
        if(thread_pdr and !thread_4){ 
            int ret = pthread_create (&tpdr1, NULL, thread_start_pdr, (void *)aiger); 
        }
        if(thread_4){ 
            int ret2 = pthread_create (&tpdr2, NULL, thread_start_pdr, (void *)aiger); 
            int ret3 = pthread_create (&tpdr3, NULL, thread_start_pdr, (void *)aiger); 
            int ret4 = pthread_create (&tpdr4, NULL, thread_start_pdr, (void *)aiger); 
        }
        for(int t = 0; t < bmc_thread; t++){
            int ret5 = pthread_create (&tbmc[t], NULL, thread_start_bmc, (void *)aiger); 
        }
        
        if(thread_pdr) 
            pthread_join(tpdr1, NULL);
        if(thread_4){
            pthread_join(tpdr2, NULL);
            pthread_join(tpdr3, NULL);
            pthread_join(tpdr4, NULL);
        }
        for(int t = 0; t < bmc_thread; t++)
            pthread_join(tbmc[t], NULL);
    }

    if(RESULT  == 10 and !no_output)
        cout << 1 << endl;
    else if(RESULT  == 20)
        cout << 0 << endl;
    //sleep(10);
    
    delete aiger;
    auto t_end = system_clock::now();
    auto duration = duration_cast<microseconds>(t_end - t_begin);
    double time_in_sec = double(duration.count()) * microseconds::period::num / microseconds::period::den;
    cout<<"c time = "<<time_in_sec<<endl;
    return 1;
}