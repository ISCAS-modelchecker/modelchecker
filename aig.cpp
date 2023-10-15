#include "aig.hpp"
#include <fstream>
#include <assert.h>
#include <cmath>
#include "basic.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

Aiger_and::Aiger_and(unsigned o, unsigned i1, unsigned i2):
    o(o),
    i1(i1),
    i2(i2)
{
}

Aiger_latches::Aiger_latches(unsigned l, unsigned next, unsigned default_val = 0):
    l(l),
    next(next),
    default_val(default_val)
{
}

Aiger::Aiger(){
    max_var = 0;
    num_inputs = 0;
    num_outputs = 0;
    num_bads = 0;
    num_ands = 0;
    num_constraints = 0;
    num_latches = 0;

    ands.clear();
    latches.clear();
    inputs.clear();
    outputs.clear();
    bads.clear();
    constraints.clear();
    symbols.clear();
}

int read_literal(unsigned char **fbuf){
    char c = **fbuf;
    while(c<'0' || c>'9'){
        if(c == '\n') return -1;
        (*fbuf)++;
        c = **fbuf;
    }

    int result = 0;
    while(c>='0' && c<='9'){
        result = result*10 + (c - '0');
        (*fbuf)++;
        c = **fbuf;
    }
    return result;
}


unsigned decode(unsigned char **fbuf){
    unsigned x = 0, i = 0;
    unsigned char ch;
    while ((ch = (*(*fbuf)++)) & 0x80){
        x |= (ch & 0x7f) << (7 * i++);
    }
    return x | (ch << (7 * i));
}

void encode (string& str, unsigned x)
{
    unsigned char ch;
    while (x & ~0x7f){
        ch = (x & 0x7f) | 0x80;
        str += ch;
        x >>= 7;
    }
    ch = x;
    str += ch;
}


Aiger* load_aiger_from_file(string str){
    Aiger *aiger = new Aiger;
    
    ifstream fin(str);
    fin.seekg(0, ios::end);
    int len = fin.tellg();
    fin.seekg(0, ios::beg);
    char *tbuf = new char[len+1];
    fin.read(tbuf, len);
    tbuf[len] = 0;
    unsigned char *fbuf = (unsigned char *)tbuf;
    fin.close();

    bool binary_mode = false;
    assert(*fbuf == 'a');
    fbuf++;
    if(*fbuf == 'i'){
        binary_mode = true;
    }else{
        assert(*fbuf == 'a');
    }
    fbuf++;
    assert(*fbuf == 'g');

    aiger->max_var      = read_literal(&fbuf);
    aiger->num_inputs   = read_literal(&fbuf);
    aiger->num_latches  = read_literal(&fbuf);
    aiger->num_outputs  = read_literal(&fbuf);
    aiger->num_ands     = read_literal(&fbuf);
    aiger->num_bads     = max(read_literal(&fbuf), 0);
    aiger->num_constraints  = max(read_literal(&fbuf), 0);
    aiger->num_justice      = max(read_literal(&fbuf), 0);
    aiger->num_fairness     = max(read_literal(&fbuf), 0);
    

    assert(aiger->max_var == (aiger->num_inputs + aiger->num_latches + aiger->num_ands));
    

    if(binary_mode){
        for(int i=1; i<=aiger->num_inputs; ++i)
            aiger->inputs.push_back(2*i);
    }else{
        for(int i=0; i<aiger->num_inputs; ++i){
            read_literal(&fbuf); fbuf++;
            aiger->inputs.push_back(read_literal(&fbuf));
        }
    }

    for(int i=0; i<aiger->num_latches; ++i){
        int l, n, d;
        read_literal(&fbuf); fbuf++;
        if(binary_mode){
            l = 2 * (aiger->num_inputs + 1 + i);
        }else{
            l = read_literal(&fbuf);
        }
        n = read_literal(&fbuf);
        d = max(read_literal(&fbuf), 0);
        // 0: reset; 1: set;  d=l: uninitialized
        aiger->latches.push_back(Aiger_latches(l, n, d));
        if(aig_veb == 2)
            printf("c read latches %d <- %d (default %d)\n", l, n, d);
    }

    for(int i=0; i<aiger->num_outputs; ++i){
        read_literal(&fbuf); fbuf++;
        aiger->outputs.push_back(read_literal(&fbuf));
    }

    for(int i=0; i<aiger->num_bads; ++i){
        read_literal(&fbuf); fbuf++;
        aiger->bads.push_back(read_literal(&fbuf));
    }

    for(int i=0; i<aiger->num_constraints; ++i){
        read_literal(&fbuf); fbuf++;
        (aiger->constraints).push_back(read_literal(&fbuf));
    }

    //cout << "initial constraints.size = " << (aiger->constraints).size() << endl;
    // // sort((aiger->constraints).begin(),(aiger->constraints).end());
    // // int kk = 0;
    // // for(vector<unsigned>::iterator itor = (aiger->constraints).begin(); itor != (aiger->constraints).end(); ){
    // //     if((kk >= 1) and (*itor == aiger->constraints[kk-1])){
    // //         itor = (aiger->constraints).erase(itor);
    // //     }
    // //     else{
    // //         itor++; kk++;
    // //     }
    // // }
    //cout << "simplified constraints.size = " << (aiger->constraints).size() << endl;
    
    // TODO: finish justice and fairness

    if(binary_mode){
        read_literal(&fbuf);fbuf++;
        int o, i1, i2, d1, d2;
        for(int i=0; i<aiger->num_ands; ++i){
            o  = 2 * (aiger->num_inputs + aiger->num_latches + i + 1);
            d1 = decode(&fbuf);
            i1 = o  - d1;
            d2 = decode(&fbuf);
            i2 = i1 - d2;
            aiger->ands.push_back(Aiger_and(o, i1, i2));
            if(aig_veb == 2)
                printf("c read and %d <- %d, %d\n", o, i1, i2);
        }

    }else{
        int o, i1, i2;
        for(int i=0; i<aiger->num_ands; ++i){
            read_literal(&fbuf); fbuf++;
            o = read_literal(&fbuf);
            i1 = read_literal(&fbuf);
            i2 = read_literal(&fbuf);
            aiger->ands.push_back(Aiger_and(o, i1, i2));
            if(aig_veb == 2)
                printf("c read and %d <- %d, %d\n", o, i1, i2);
        }

    }

    // read symbols
    string tstr;
    bool comments = false;
    if(!binary_mode)
        while(*fbuf != '\n') {fbuf++;}
    if((char *)fbuf - tbuf < len-1){
        if(binary_mode) fbuf--;
        while(true){
            fbuf++;
            tstr = "";
            if(*fbuf == 'i'){
                int v = read_literal(&fbuf);
                fbuf++;
                while(*fbuf != '\n'){
                    tstr += *fbuf;
                    fbuf++;
                }
                aiger->symbols[v] = tstr;
                if(aig_veb == 2)
                    cout << "i" << v << " " << tstr << endl;
            }else if(*fbuf == 'l'){
                int v = read_literal(&fbuf);
                fbuf++;
                while(*fbuf != '\n'){
                    tstr += *fbuf;
                    fbuf++;
                }
                aiger->symbols[v] = tstr;
                if(aig_veb == 2)
                    cout<< "l" << v<<" "<<tstr<<endl;
            }else if(*fbuf == 'o'){
                int v = read_literal(&fbuf);
                fbuf++;
                while(*fbuf != '\n'){
                    tstr += *fbuf;
                    fbuf++;
                }
                aiger->symbols[v] = tstr;
                if(aig_veb == 2)
                    cout<<"o"<<v<<" "<<tstr<<endl;
            }else if(*fbuf == 'c'){
                comments = true;
                fbuf++;
                break;
            }else{
                break;
            }
        }
    }

    aiger->comments = "";
    if(comments){
        assert(*fbuf == '\n');
        fbuf++;
        aiger->comments = string((char*)fbuf);
        if(aig_veb == 2)
            cout<<aiger->comments<<endl;
    }    
    if(aig_veb){
        printf("c finish parse with M %d I %d L %d O %d A %d B %d C %d J %d F %d\n"
            , aiger->max_var
            , aiger->num_inputs
            , aiger->num_latches
            , aiger->num_outputs
            , aiger->num_ands
            , aiger->num_bads
            , aiger->num_constraints
            , aiger->num_justice
            , aiger->num_fairness);
    }
    return aiger;
}