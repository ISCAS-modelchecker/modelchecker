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

void *thread_start(void *arg){
    int arg_data = *((int *)arg);
    for(int i=0; i<5; i++){
        for(int j=0; j<5; j++);
        printf("receive arg %d = %d\n", arg_data, arg_data);
    }  
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
    pthread_t tid;
    int arg1 = 1;
    int arg2 = 2;
    int ret = pthread_create (&tid, NULL, thread_start, (void *)&arg1);
    thread_start ((void *) &arg2);    

    auto t_begin = system_clock::now();

    cout<<"c USAGE: ./modelchecker <aig-file> [propertyIndex]"<<endl;
    Aiger *aiger = load_aiger_from_file(string(argv[1]));
    int property_index = 0;
    if(argc > 2){
        property_index = (unsigned) atoi(argv[2]);
    }
         
    int nframes = 10;
    BMC bmc(aiger, property_index, nframes);
    bmc.initialize();
    int res_bmc = bmc.check(); 
    cout << res_bmc << endl;

    PDR pdr(aiger, property_index);
    bool res = pdr.check();   
    cout << res << endl;

    
    delete aiger;
    auto t_end = system_clock::now();
    auto duration = duration_cast<microseconds>(t_end - t_begin);
    double time_in_sec = double(duration.count()) * microseconds::period::num / microseconds::period::den;
    cout<<"c time = "<<time_in_sec<<endl;
    return 1;
}