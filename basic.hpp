#pragma once
#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>

//for PDR enhancement
#define use_heuristic 1
#define use_earliest_strengthened_frame 1
#define use_punishment 1
#define split_ands 0         //0 better
#define assumption_allsort 0 //0 better
#define restart 0            //Temporarily 0

//for interaction
#define share_memory 0     
#define share_memory_test 0   

//for PDR log
#define moreinfo_for_pdr 0
#define output_witness_with_latch 0
#define output_stats_for_recblock 0
#define output_stats_for_heuristic 0
#define output_stats_for_extract 0
#define output_stats_for_propagate 0
#define output_stats_for_addcube 0
#define output_stats_for_frames 0
#define output_stats_for_ctg 0
#define output_stats_for_others 0
#define output_stats_for_conclusion 0
#define output_stats_for_cpo 0

//for BMC log
#define aig_veb 0
#define moreinfo_for_bmc 1
#define unfold_ands 0
#define unfold_latches 0
#define output_aigand 0

class Literal{
public:
    int val;
    Literal(){};
    Literal(int val):val(val){};


    bool operator == (Literal p) const { return val == p.val; }
    bool operator != (Literal p) const { return val != p.val; }
    bool operator <  (Literal p) const { return val < p.val;  } // '<' makes p, ~p adjacent in the ordering.

    friend Literal mkLit(unsigned var, bool sign);
};

inline  Literal  mkLit      (unsigned var, bool sign = false) { Literal p; p.val = var + var + (int)sign; return p; }

inline  Literal operator~   (Literal p)              { Literal q; q.val = p.val ^ 1; return q; }
inline  Literal operator^   (Literal p, bool b)      { Literal q; q.val = p.val ^ (unsigned int)b; return q; }
inline  bool    get_sign    (Literal p)              { return p.val & 1; }
inline  int     get_var     (Literal p)              { return p.val >> 1; }


class lbool {
    short value;

public:
    explicit lbool(short v) : value(v) { }

    lbool()       : value(0) { }
    explicit lbool(bool x) : value(!x) { }

    bool  operator == (lbool b) const { return ((b.value&2) & (value&2)) | (!(b.value&2)&(value == b.value)); }
    bool  operator != (lbool b) const { return !(*this == b); }
    lbool operator ^  (bool  b) const { return lbool((short)(value^(short)b)); }

    lbool operator && (lbool b) const { 
        short sel = (this->value << 1) | (b.value << 3);
        short v   = (0xF7F755F4 >> sel) & 3;
        return lbool(v); }

    lbool operator || (lbool b) const {
        short sel = (this->value << 1) | (b.value << 3);
        short v   = (0xFCFCF400 >> sel) & 3;
        return lbool(v); }

    friend int   toInt  (lbool l);
    friend lbool toLbool(int   v);
};

inline int   toInt  (lbool l) { return l.value; }
inline lbool toLbool(int   v) { return lbool((short)v);  }

const lbool l_True ((short)0);
const lbool l_False((short)1);
const lbool l_Undef((short)2);



// variable 1 is const True.
inline int aiger_to_dimacs(int lit){
    int res = lit >> 1;
    if(lit & 1)
        return -res-1;
    else
        return res+1;
}

inline int dimacs_to_aiger(int lit){
    int res = (abs(lit) - 1)<<1;
    if(lit < 0){
        return res + 1;
    }else{
        return res;
    }
}

inline bool isNumber(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

inline int validateInput(const std::string& arg, const std::string& value, int min, int max, int defaultValue) {
    if (!isNumber(value)) {
        std::cerr << "Warning: " << arg << " value must be a number. Setting to " << defaultValue << "." << std::endl;
        return defaultValue;
    }

    int num = std::atoi(value.c_str());
    if (num < min || num > max) {
        std::cerr << "Warning: " << arg << " value must be between " << min << " and " << max << ". Setting to " << defaultValue << "." << std::endl;
        return defaultValue;
    }
    return num;
}