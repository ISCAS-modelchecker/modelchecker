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
extern "C" {
#include "fold.h"
}
using namespace std;
using namespace std::chrono;

int thread_count = 0;

void *thread_start_bmc(void *aiger){
    int nframes = INT_MAX;
    Aiger* aiger_data = ((Aiger *)aiger); 
    BMC bmc(aiger_data, 0, nframes);
    bmc.initialize();
    int res_bmc = bmc.check(); 
    if(!no_output) cout << "res_bmc = " << res_bmc << endl;
    pthread_exit(NULL); 
}

void *thread_start_pdr(void *aiger){
    int thread_index = thread_count++;
    Aiger* aiger_data = ((Aiger *)aiger);
    PDR pdr(aiger_data, thread_index);
    int res_pdr = pdr.check();   
    if(!no_output) cout << "res_pdr" + to_string(thread_index) +  "= " + to_string(res_pdr) + "\n";
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
    //srand((unsigned int)time(NULL));
    //freopen("freopen.out","w",stdout);
    auto t_begin = system_clock::now();
    if(!no_output) cout<<"c USAGE: ./modelchecker <aig-file> [propertyIndex]\n";
    Aiger *aiger = load_aiger_from_file(string(argv[1]));
    //char *fold = aiger_fold(argv[1], 33333331);
    //printf("%s\n", fold);
    //Aiger *aiger = load_aiger_from_file(string(fold));
    int property_index = 0;
    if(argc > 2){
        property_index = (unsigned) atoi(argv[2]);
    }
         
    pthread_t tpdr1, tpdr2, tpdr3, tpdr4, tbmc;
    
    int ret = pthread_create (&tpdr1, NULL, thread_start_pdr, (void *)aiger); 
    if(thread_4){
        int ret2 = pthread_create (&tpdr2, NULL, thread_start_pdr, (void *)aiger); 
        int ret3 = pthread_create (&tpdr3, NULL, thread_start_pdr, (void *)aiger); 
        int ret4 = pthread_create (&tpdr4, NULL, thread_start_pdr, (void *)aiger); 
    }
    if(thread_8){
        int ret5 = pthread_create (&tbmc, NULL, thread_start_bmc, (void *)aiger); 
    }

    pthread_join(tpdr1, NULL);
    if(thread_4){
        pthread_join(tpdr2, NULL);
        pthread_join(tpdr3, NULL);
        pthread_join(tpdr4, NULL);
    }
    if(thread_8){
        pthread_join(tbmc, NULL);
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