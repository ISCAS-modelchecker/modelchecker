#include "aig.hpp"

std::mutex result_mutex;  
unsigned long long state_count = 0;
int RESULT = 0; // 0 means safe in PORTFOLIO; 10 finds a bug; 20 proves safety

Aiger::Aiger(){
    max_var = num_inputs = num_outputs = num_bads = num_ands = num_constraints = num_latches = binaryMode = 0;

    Aiger_ands.clear();
    Aiger_latches.clear();
    Aiger_inputs.clear();
    Aiger_outputs.clear();
    Aiger_bads.clear();
    Aiger_constraints.clear();
    symbols.clear();

    //for MCbase
    variables.clear();
    ands.clear();
    nexts.clear();
    init_state.clear();
    constraints.clear();
    constraints_prime.clear();
    allbad.clear();

    //for ic3base
    map_to_prime.clear();
    map_to_unprime.clear();
}

void Aiger::translate_to_dimacs(){
    variables.push_back(Variable(0, string("NULL")));
    variables.push_back(Variable(1, string("False")));

    // load inputs
    for(int i=1; i<=num_inputs; ++i){
        assert((i)*2 == Aiger_inputs[i-1]);
        variables.push_back(Variable(1 + i, 'i', i-1, false));
    }

    // load latches
    for(int i=1; i<=num_latches; ++i){
        assert((num_inputs + i)*2 == Aiger_latches[i-1].l);
        variables.push_back(Variable(1 + num_inputs + i, 'l', i-1, false));
    }

    // load ands
    for(int i=1; i<=num_ands; ++i){
        assert(2*(num_inputs + num_latches + i) == Aiger_ands[i-1].o);
        int o = 1 + num_inputs + num_latches + i;
        int i1 = aiger_to_dimacs(Aiger_ands[i-1].i1);
        int i2 = aiger_to_dimacs(Aiger_ands[i-1].i2);
        variables.push_back(Variable(o, 'a', i-1, false));
        ands.push_back(And(o, i1, i2));
    }

    // deal with initial states
    for(int i=1; i<=num_latches; ++i){
        int l = 1 + num_inputs + i;
        assert((l-1)*2 == Aiger_latches[i-1].l);
        Aiger_latch & al = Aiger_latches[i-1];
        nexts.push_back(aiger_to_dimacs(al.next));
        if(al.default_val==0){
            init_state.push_back(-l);
        }else if(al.default_val==1){
            init_state.push_back(l);
        }
    }

    // deal with constraints
    for(int i=0; i<num_constraints; ++i){
        int cst = Aiger_constraints[i];
        constraints.push_back(aiger_to_dimacs(cst));
    }

    // load bad states
    if(num_bads > 0){
        if (num_bads <= propertyIndex){
            cerr << "Warning: property_index out of range (must be between 0 and " << num_bads - 1 << "). Setting to 0." << std::endl;
            propertyIndex = 0;
        }
        bad = aiger_to_dimacs(Aiger_bads[propertyIndex]);
        for(int i=0; i<num_bads; i++){
            allbad.push_back(aiger_to_dimacs(Aiger_bads[i]));
        }
    }else if(num_outputs > 0){
        cerr << "Warning: The outputs have been regarded as bads as they are not empty." << std::endl;
        if (num_outputs <= propertyIndex){
            cerr << "Warning: property_index out of range (must be between 0 and " << num_outputs - 1 << "). Setting to 0." << std::endl;
            propertyIndex = 0;
        }        
        bad = aiger_to_dimacs(Aiger_outputs[propertyIndex]);
        for(int i=0; i<num_outputs; i++){
            allbad.push_back(aiger_to_dimacs(Aiger_outputs[i]));
        }
    }else{
        cerr << "Error: No properties to verify (no bad states or outputs declared in the circuit). Exiting." << std::endl;
        assert(false);
    }
    assert(abs(bad) <= variables.size());

    //prepare for prime
    primed_first_dimacs = variables.size();
    assert(primed_first_dimacs == 1 + num_inputs + num_latches + num_ands + 1);
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

void encode (string& str, unsigned x){
    unsigned char ch;
    while (x & ~0x7f){
        ch = (x & 0x7f) | 0x80;
        str += ch;
        x >>= 7;
    }
    ch = x;
    str += ch;
}


Aiger* load_aiger_from_file(string str, int pi, bool witness, bool certificate){
    Aiger *aiger = new Aiger;
    aiger->propertyIndex = pi;
    aiger->output_witness = witness;
    aiger->output_certificate = certificate;
    
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
    aiger->binaryMode = 0;
    assert(*fbuf == 'a');
    fbuf++;
    if(*fbuf == 'i'){
        binary_mode = true;
        aiger->binaryMode = 1;
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
            aiger->Aiger_inputs.push_back(2*i);
    }else{
        for(int i=0; i<aiger->num_inputs; ++i){
            read_literal(&fbuf); fbuf++;
            aiger->Aiger_inputs.push_back(read_literal(&fbuf));
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
        aiger->Aiger_latches.push_back(Aiger_latch(l, n, d));
        if(aig_veb == 2)
            printf("c read latches %d <- %d (default %d)\n", l, n, d);
    }

    for(int i=0; i<aiger->num_outputs; ++i){
        read_literal(&fbuf); fbuf++;
        aiger->Aiger_outputs.push_back(read_literal(&fbuf));
        if(aig_veb == 2)
            printf("c read outputs %d\n", aiger->Aiger_outputs[i]);
    }

    for(int i=0; i<aiger->num_bads; ++i){
        read_literal(&fbuf); fbuf++;
        aiger->Aiger_bads.push_back(read_literal(&fbuf));
        if(aig_veb == 2)
            printf("c read bads %d\n", aiger->Aiger_bads[i]);
    }

    for(int i=0; i<aiger->num_constraints; ++i){
        read_literal(&fbuf); fbuf++;
        (aiger->Aiger_constraints).push_back(read_literal(&fbuf));
        if(aig_veb == 2)
            printf("c read constraint %d\n", aiger->Aiger_constraints[i]);
    }
    
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
            aiger->Aiger_ands.push_back(Aiger_and(o, i1, i2));
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
            aiger->Aiger_ands.push_back(Aiger_and(o, i1, i2));
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