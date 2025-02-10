#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <assert.h> //#include <cassert>
#include "basic.hpp"
#ifndef TIMESTAMP
#define TIMESTAMP
    extern unsigned long long state_count;
    extern int RESULT; // 0 means safe in PORTFOLIO; 10 finds a bug; 20 proves safety
    extern std::mutex result_mutex;  
#endif

using namespace std;

class Aiger_and{
public:
    Aiger_and(unsigned o, unsigned i1, unsigned i2): o(o), i1(i1), i2(i2){}
    unsigned o, i1, i2;
};

class Aiger_latch{
public:
    Aiger_latch(unsigned l, unsigned next, unsigned default_val = 0): l(l), next(next), default_val(default_val){}
    unsigned l, next, default_val;
};

class And{
public:
    int o, i1, i2;
    And(int o, int i1, int i2):o(o),i1(i1),i2(i2){}
};

// save information for debug
class Variable{ 
public:
    int dimacs_var; // the index in variables set;
    string name;    // the name of this variable
    Variable(int dimacs_index, string name){
        this->dimacs_var = dimacs_index;
        this->name = name;
    }
    Variable(int dimacs_index, char type, int type_index, bool prime){
        this->dimacs_var = dimacs_index;
        assert(type=='i' || type=='o' || type=='l' || type == 'a');
        stringstream ss;
        ss << type;
        ss << type_index;
        if(prime)
            ss << "'";
        ss >> name;
}
};

// not support justice and fairness now;
class Aiger{
public:
    //for aig input
    int max_var;           // the max number of variables
    int num_inputs;        // the number of input variables
    int num_outputs;       // the number of output variables
    int num_latches;       // the number of latches
    int num_ands;          // the number of ands
    int num_bads;          // the number of bads, not hope to hold for any step.
    int num_constraints;   // the number of constraints, holds for all the time span.
    // TODO: apply for justice & fairness
    int num_justice;       // the number of justices.
    int num_fairness;      // the number of fairness.
    bool binaryMode;

    vector<unsigned> Aiger_inputs, Aiger_outputs, Aiger_bads, Aiger_constraints;
    vector<Aiger_latch> Aiger_latches;
    vector<Aiger_and> Aiger_ands;
    map<unsigned, string> symbols;
    string comments;

    //for MCbase
    vector<Variable> variables;
    vector<And> ands;
    vector<int> nexts, init_state;  //set<int> set_init_state;
    vector<int> constraints, constraints_prime;
    vector<int> allbad;
    int bad, bad_prime;

    //for ic3base
    map<int, int> map_to_prime, map_to_unprime; // used for mapping ands
    const int unprimed_first_dimacs = 2;



    Aiger();
};

Aiger* load_aiger_from_file(string str);