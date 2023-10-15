#pragma once

#include <vector>
#include <iostream>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <assert.h>
#include <algorithm>
#include "BMC.hpp"
using namespace std;

class BMPDR{
public:    
    BMC *bmc_;
    PDR *pdr_;
    int frame_index;

    BMPDR(BMC *bmc, PDR *pdr):bmc_(bmc), pdr_(pdr){
        frame_index = 0;
    }

    void printcube(){
        int k=0;
        for(Frame & f: pdr_->frames){
            cout << "Level " << k++ <<" (" << f.cubes.size();
            cout<<" v "<<f.solver->max_var()<<" ";
            cout << ") :" << endl;
            for(const Cube &c : f.cubes){
                Cube cc = c;
                pdr_->show_litvec(cc);
            }
            if(k == (pdr_->frames).size()-1) break;
        }
    }

    void printinfo_pdr(){
        cout << "PDR finish depth " << (pdr_->depth())-1 << endl;
    }

    void addcube(){
        Frame f = (pdr_->frames).at(pdr_->depth()-1);
        for(const Cube &c : f.cubes){
            Cube cc = c;
            //pdr_->show_litvec(cc);
            for(int l:cc){ 
                int unfold_l = (l > 0 ? (bmc_->values[l]) : -(bmc_->values[-l]));
                (bmc_->bmcSolver)->add(-unfold_l); 
            }
            (bmc_->bmcSolver)->add(0);  
        }
    }

    int check(){
        bmc_->initialize(); 
        frame_index = 0;       
        // while(true){ 
        //     frame_index++;  
        //     /// PDR
        //     cout << "PDR start frame " << frame_index << endl;    
        //     int pdr_res = pdr_->incremental_check();
        //     if(pdr_res != -1)   return pdr_res;
        //     //printcube();

        //     /// BMC     
        //     cout << "BMC start frame " << frame_index << endl;     
        //     bmc_->unfold();
        //     //addcube(); 
        //     bmc_->bmc_frame_k++;
        //     int bmc_res = (bmc_->solve_one_frame());
        //     if(bmc_res == 10){
        //         (bmc_->uaiger)->show_statistics();
        //         cout << "Output was asserted in frame." << endl;
        //         return 1; 
        //     }    
        //     cout << endl;  
        // } 

        while(true){ 
            frame_index++;  
            /// PDR
            cout << "PDR start frame " << frame_index << endl;    
            int pdr_res = pdr_->incremental_check();
            if(pdr_res != -1)   return pdr_res;
            //printcube();

            /// BMC     
            cout << "BMC start frame " << frame_index << " " << frame_index+1 << endl;     
            bmc_->unfold();
            //addcube(); 
            bmc_->bmc_frame_k++;
            bmc_->unfold();
            bmc_->bmc_frame_k++;
            int bmc_res = (bmc_->solve_one_frame());
            if(bmc_res == 10){
                (bmc_->uaiger)->show_statistics();
                cout << "Output was asserted in frame." << endl;
                return 1; 
            }    
            //cout << endl;  

            frame_index++;  
            /// PDR
            cout << "PDR start frame " << frame_index << endl;    
            pdr_res = pdr_->incremental_check2();
            if(pdr_res != -1)   return pdr_res;  
            cout << endl;            
        } 
    }
};