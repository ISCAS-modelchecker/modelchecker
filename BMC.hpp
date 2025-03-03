#pragma once

#include <chrono>
#include <algorithm>
#include <set>
#include "aig.hpp"
#include "sat_solver.hpp"

class Node{
public:
    int type;            // 0:null 1:const 2:input 3:and
    int fathers;         // father node count
    int child1, child2;  // child node
    bool activate; 

    Node(){
        type = fathers = child1 = child2 = 0;
    }
    Node(int type, int fathers, int child1, int child2):type(type),fathers(fathers),child1(child1),child2(child2){activate = 1;}
};

class UnfoldAiger{
public:
    vector<int> inputs, outputs, constraints, init;
    vector<And> ands;
    vector<Node> nodes;
    
    vector<vector<And>> hash_table; // hash_table[k] stores AND gate <output, k, neighbor> (store father of each child node)

    UnfoldAiger(){
        ands.clear(); inputs.clear(); outputs.clear(); constraints.clear();
    }

    int asize(){return ands.size();}
    int isize(){return inputs.size();}
    int osize(){return outputs.size();}
    int nsize(){return nodes.size();}

    void CleanupFrame(){
        int oldsize = nodes.size();
        for(int i=oldsize-1; i>=0; i--){
            if(nodes[i].fathers == 0 and nodes[i].type == 3) {
                nodes[i].activate = 0;
                int i1 = abs(nodes[i].child1);
                int i2 = abs(nodes[i].child2);
                nodes[i1].fathers--;
                nodes[i2].fathers--;
            }
        }

        //show_ands();
        ands.clear();
        for(int i=0; i<oldsize; i++){
            if(nodes[i].type == 3 and nodes[i].activate == 1){
                ands.push_back(And(i, nodes[i].child1, nodes[i].child2));
            }
        }
    }

    void show_statistics(){
        cout << "inputs size = " << isize() << "  ";
        cout << "ouputs size = " << osize() << "  ";
        cout << "ands size = " << asize() << "  ";
        cout << "nodes size = " << nsize() << "  ";
        cout << endl;
    }

    void show_constraints(){
        cout << "constraints: ";
        for(int l: constraints) 
            cout << l << " ";
        cout << endl;
    }

    void show_ands(){
        cout << "ands: ";
        for(int i=0; i<ands.size(); i++){
            cout << ands[i].o << " " << ands[i].i1 << " " <<  ands[i].i2 << endl;
            if(i > 1000 ) break;
        }
        cout << endl;
    }
};

class BMC : public Aiger
{
public:
    UnfoldAiger *uaiger;
    
    // the interal data structure for Aiger (in CNF dimacs format).
    
    //for BMC unfold
    int nframes, max_nodes;
    vector<int> values;   // "real value of each node" corresponds to "variables"
    int tempvalue[999999];
    bool check_init;

    //for BMC solve
    CaDiCaL *bmcSolver = nullptr;
    int bmc_frame_k;
    int max_index_of_ands_added_to_solver; 
    vector<bool> lit_has_insert; 
    
    // Parameters & statistics
    std::chrono::_V2::steady_clock::time_point start_time;

    // Parallel
    int thread_index;
    int max_thread_index;

    BMC(Aiger *aiger, int nframes, int Thread_index, int max): Aiger(*aiger), nframes(nframes), thread_index(Thread_index), max_thread_index(max){
        start_time = std::chrono::steady_clock::now();  
        check_init = 0;
        max_index_of_ands_added_to_solver =0;
        max_nodes = INT_MAX;
    }
    ~BMC(){
        if(bmcSolver != nullptr) delete bmcSolver;
    }

    // Main BMC framework
    void initialize(); 
    int check();
    void check_one_frame();
    int Aig_And(int lc, int rc);
    void unfold();
    double get_runtime();
    void encode_init_condition(SATSolver *s);
    int solve();
    int solve_one_frame();

    // log
    void show_bads();
    void show_constraints();
    void show_aag();
    void show_variables();
    void show_ands();
    void show_nexts();
    void show_values();
    void show_lit(int l) const;
    void show_litvec(vector<int> &lv) const;

    static int terminate_callback(void * state) {
        return (RESULT != 0);  // Return the value of the global variable 'result'
    }
};
