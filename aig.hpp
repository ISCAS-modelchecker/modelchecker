#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

class Aiger_and{
public:
    Aiger_and(unsigned o, unsigned i1, unsigned i2);
    unsigned o, i1, i2;
};

class Aiger_latches{
public:
    Aiger_latches(unsigned l, unsigned next, unsigned default_val);
    unsigned l, next, default_val;
};

// not support justice and fairness now;
class Aiger{
public:
    unsigned max_var;           // the max number of variables
    unsigned num_inputs;        // the number of input variables
    unsigned num_outputs;       // the number of output variables
    unsigned num_latches;       // the number of latches
    unsigned num_ands;          // the number of ands
    unsigned num_bads;          // the number of bads, not hope to hold for any step.
    unsigned num_constraints;   // the number of constraints, holds for all the time span.
    // TODO: apply for justice & fairness
    unsigned num_justice;       // the number of justices.
    unsigned num_fairness;      // the number of fairness.

    vector<unsigned> inputs, outputs, bads, constraints;
    vector<Aiger_latches> latches;
    vector<Aiger_and> ands;
    map<unsigned, string> symbols;
    string comments;

    Aiger();
};

Aiger* load_aiger_from_file(string str);