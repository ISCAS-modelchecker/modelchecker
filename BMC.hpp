#pragma once

#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include "aig.hpp"
#include "basic.hpp"
#include "sat_solver.hpp"
#include "PDR.hpp"
using namespace std;

class Node{
public:
    int type;            // 0:null 1:const 2:input 3:and
    int fathers;         // 父节点个数
    int child1, child2;  // 子节点
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
    vector<Variable> unfold_variables;
    vector<Node> nodes;
    
    vector<vector<And>> hash_table; // hash_table[k] 存储与门<output, k, neighbor> (存储每个子节点的父亲)

    UnfoldAiger(){
        unfold_variables.clear(); ands.clear(); inputs.clear(); outputs.clear(); constraints.clear();
    }

    int vsize(){return unfold_variables.size();}
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
            //cout << i << " " << nodes[i].child1 << nodes[i].child2 << endl;
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

    void show_unfold_variable(){
        cout << "-------------show_unfold_variables------------------" <<endl;
        int ct = 1;
        int sz = unfold_variables.size();
        for(int i=0; i<sz; ++i){
            Variable &v = unfold_variables[i];
            cout << "unfold_variable[" << i << "]= v" << v.dimacs_var << "(" << v.name << ")    \t";
            if(ct++ % 4 == 0 || (i+1<sz && unfold_variables[i+1].name[0] != v.name[0])){
                cout << endl;
                ct = 1;
            }
        }
        cout << endl;
    }
};

class BMC
{
public:
    Aiger *aiger;
    UnfoldAiger *uaiger;
    
    // the interal data structure for Aiger (in CNF dimacs format).
    int nInputs, nLatches, nAnds;
    vector<Variable> variables;
    vector<And> ands;
    vector<int> nexts;
    vector<int> constraints, constraints_prime;
    vector<int> init_state; set<int> set_init_state;
    vector<int> allbad;
    int bad, bad_prime;
    const int unprimed_first_dimacs = 2;
    int primed_first_dimacs;
    int property_index;
    map<int, int> map_to_prime, map_to_unprime; // used for mapping ands
    
    //for BMC unfold
    int nframes;
    vector<int> values;   // real value of each node 与variables一一对应
    int tempvalue[999999];
    bool check_init;

    //for BMC solve
    CaDiCaL *bmcSolver = nullptr;
    int bmc_frame_k;
    vector<bool> lit_has_insert; 
    
    // Parameters & statistics
    std::chrono::_V2::steady_clock::time_point start_time;

    BMC(Aiger *aiger, int property_index, int nframes): aiger(aiger), property_index(property_index), nframes(nframes){
        start_time = std::chrono::steady_clock::now();  
        allbad.clear();
        check_init = 0;
    }
    ~BMC(){
        if(bmcSolver != nullptr) delete bmcSolver;
    }

    // Aiger
    void translate_to_dimacs();

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
};
