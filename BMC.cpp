#include "BMC.hpp"
#define value(rc) (rc > 0 ? values[rc] : -values[-rc])  //primitive variables ->unfold variables

//  Log functions
// --------------------------------------------
double BMC::get_runtime(){
    auto stop_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop_time - start_time;
    return elapsed_seconds.count();
}

void BMC::show_variables(){
    cout << "-------------show_variables------------------" <<endl;
    int ct = 1;
    for(int i=1; i<variables.size(); ++i){
        Variable &v = variables[i];
        // assert(i == v.dimacs_var);
        cout << "variable[" << i << "]= v" << v.dimacs_var << "(" << v.name << ")    \t";
        if(ct++ % 20 == 0 || (i+1<variables.size() && variables[i+1].name[0] != v.name[0])){
            cout << endl;
            ct = 1;
        }
    }
    cout << endl;
}

void BMC::show_ands(){
    cout << "-------------show_ands------------------" <<endl;
    cout << "num_ands = " << num_ands << ", ands.size() = " << ands.size() << endl;
    for(int i=0; i<ands.size(); ++i){
        cout << "ands[" << i << "] = " << ands[i].o << " " << ands[i].i1 << " " << ands[i].i2 <<endl;
        if(i>1000) break;
    }
}

void BMC::show_nexts(){
    cout << "-------------show_nexts_inits------------------" <<endl;
    cout << "num_latches = " << num_latches << ", nexts.size() = " << nexts.size() << ", inits.size() = " << init_state.size() << endl;
    for(int i=0; i<nexts.size(); ++i){
        cout << "nexts[" << i << "] = " << nexts[i] << " ";
    }
    cout << endl;
    for(int i=0; i<init_state.size(); ++i){
        cout << "init_state[" << i << "] = " << init_state[i] << " ";
    }
    cout << endl;
}

void BMC::show_values(){
    cout << "-------------show_values------------------" <<endl;
    cout << "values.size() = " << values.size() << endl;
    for(int i=0; i<values.size(); ++i){
        cout << "values[" << i << "] = " << values[i] << " ";
    }
    cout << endl;
}

void BMC::show_bads(){
    cout << "-------------show_BADs------------------" <<endl;
    cout << "bad = " << bad <<endl;
}

void BMC::show_constraints(){
    cout << "-------------show_constraints------------------" <<endl;
    cout << "constraints size = " << constraints.size() << endl;
    for(int i=0; i<constraints.size(); i++){
        cout << constraints[i] << " ";
    }
    cout << endl;
}

void BMC::encode_init_condition(SATSolver *s){
    s->add(-1); s->add(0); 
    for(int l : init_state){ 
        bmcSolver->add(l); bmcSolver->add(0);
    }

    if(constraints.size() >= 0){
        for(int l : constraints){ 
            s->add(l); s->add(0);}

        set<int> lit_set;
        for(int l : constraints)
            lit_set.insert(abs(l));

        for(auto i = ands.rbegin(); i != ands.rend(); ++i){
            And & a = *i;
            if(lit_set.find(a.o) == lit_set.end())
                continue;
            lit_set.insert(abs(a.i1));
            lit_set.insert(abs(a.i2));
            
            s->add(-a.o); s->add(a.i1);  s->add(0);
            s->add(-a.o); s->add(a.i2);  s->add(0);
            s->add(a.o);  s->add(-a.i1); s->add(-a.i2); s->add(0);
        }
    }
}

// LC and RC are two child nodes of an AND gate, which can be positive or negative, and can be latch/input
int BMC::Aig_And(int p0, int p1){  // input primitive var p0, p1, return unfold var p0 * p1
    int first_ands_index = num_inputs + num_latches + 2;     // = ands[0].o
    int v0 = value(p0), v1 = value(p1);
    
    //check trivial cases 
    if ( v0 == v1 )
        { return v0;}
    if ( v0 == -v1 )
        { return 1;}
    if ( abs(v0) == 1 )
        { return v0 == -1 ? v1 : 1;}
    if ( abs(v1) == 1 )
        { return v1 == -1 ? v0 : 1;} 

    // check not so trivial cases: p0 or p1 AND gate
    int pfana, pfanb, pfanc, pfand, va, vb, vc, vd;    //grandson nodes, positive/negative, latch/input. pdana primitive var, va unfold var
    if (abs(p0) >= first_ands_index){ //p0 AND gate
        pfana = ands[abs(p0) - first_ands_index].i1; va = value(pfana);
        pfanb = ands[abs(p0) - first_ands_index].i2; vb = value(pfanb);
    }
    else{                             //p0 latch/input
        pfana = abs(p0); va = value(pfana);
        pfanb = -1;      vb = value(pfanb);
    }
    if (abs(p1) >= first_ands_index){ //p1 AND gate
        pfanc = ands[abs(p1) - first_ands_index].i1; vc = value(pfanc);
        pfand = ands[abs(p1) - first_ands_index].i2; vd = value(pfand);
    }
    else{                             //p0 latch/input
        pfanc = abs(p1); vc = value(pfanc);
        pfand = -1;      vd = value(pfand);
    }

    if (abs(p0) >= first_ands_index || abs(p1) >= first_ands_index){
        if (p0 < 0){
            if ( va == -v1 || vb == -v1 )
                {if(output_aigand) cout << "strash"; return v1;}
            if ( vb == v1 )
                {if(output_aigand) cout << "strash"; return Aig_And( -pfana, pfanb );} 
            if ( va == v1 )
                {if(output_aigand) cout << "strash"; return Aig_And( -pfanb, pfana );} 
        }
        else if(p0 > 0){
            if ( va == -v1 || vb == -v1 )
                {if(output_aigand) cout << "strash"; return 1;}
            if ( va == v1 || vb == v1 )
                {if(output_aigand) cout << "strash"; return v0;}
        }

        if (p1 < 0){
            if ( vc == -v0 || vd == -v0 )
                {if(output_aigand) cout << "strash"; return v0;}
            if ( vd == v0 )
                {if(output_aigand) cout << "strash"; return Aig_And( -pfanc, pfand );} 
            if ( vc == v0 )
                {if(output_aigand) cout << "strash"; return Aig_And( -pfand, pfanc );} 
        }
        else if(p1 > 0){
            if ( vc == -v0 || vd == -v0 )
                {if(output_aigand) cout << "strash"; return 1;}
            if ( vc == v0 || vd == v0 )
                {if(output_aigand) cout << "strash"; return v1;}
        }
    }
    return 0;
}

void BMC::unfold(){ 
    //deal with inputs
    for(int i=0; i<num_inputs; ++i){
        uaiger->nodes.push_back(Node(2, 0, 0, 0));     
        uaiger->inputs.push_back(uaiger->nsize()-1);
        values[i+2] = uaiger->nsize()-1;
    }

    //deal with ands o = ia * ib
    if(unfold_ands) cout << "unfold ands" << endl;
    for(int i=0; i<num_ands; ++i){
        assert(ands[i].o == i+num_inputs+num_latches+2);
        values[ands[i].o] = 0;

        //prioritize AND gate simplification
        values[ands[i].o] = Aig_And(ands[i].i1, ands[i].i2);
        if (unfold_ands and values[ands[i].o] != 0 and i<500){
            cout << ands[i].o << " " << ands[i].i1 << " " << ands[i].i2 << "->";
            cout << "values[" << ands[i].o << "] = " << values[ands[i].o] << endl;
            continue;
        }
        
        //simplification failure, try to calculate a new AND gate
        if (values[ands[i].o] == 0){
            int i1, i2, output = 0;
            i1 = value(ands[i].i1);
            i2 = value(ands[i].i2);
            // Check whether the parent node of i1 has another child node i2 with the same value (equivalence verification)
            for(int k=0; k<(uaiger->hash_table[abs(i1)]).size(); k++ ){
                if( (uaiger->hash_table[abs(i1)][k]).i1 == i1 && (uaiger->hash_table[abs(i1)][k]).i2 == i2){
                    values[i+num_inputs+num_latches+2] = (uaiger->hash_table[abs(i1)][k]).o;
                    output = (uaiger->hash_table[abs(i1)][k]).o;
                }
                else if( (uaiger->hash_table[abs(i1)][k]).i2 == i1 && (uaiger->hash_table[abs(i1)][k]).i1 == i2){
                    values[i+num_inputs+num_latches+2] = (uaiger->hash_table[abs(i1)][k]).o;
                    output = (uaiger->hash_table[abs(i1)][k]).o;
                }  
            }
            // no existing equivalent node
            if (values[ands[i].o] == 0){
                // fail to simplify ands[i], new AND gate
                uaiger->nodes.push_back(Node(3, 0, i1, i2));        
                uaiger->nodes[abs(i1)].fathers++;
                uaiger->nodes[abs(i2)].fathers++;

                values[i+num_inputs+num_latches+2] = uaiger->nsize()-1;
                output = uaiger->nsize()-1;

                (uaiger->ands).push_back(And(output, i1, i2));
                (uaiger->hash_table[abs(i1)]).push_back(And(output, i1, i2));
                (uaiger->hash_table[abs(i2)]).push_back(And(output, i1, i2));
            }
            //if(and_is_cons == 1)  uaiger->nodes[abs(values[ands[i].o])].fathers++;
            if(unfold_ands && i<500)    cout << ands[i].o << " " << ands[i].i1 << " " << ands[i].i2 << "->" << output << " " << i1 << " " << i2 << endl;   
        }
    }
    //deal with constraints
    for(int i=0; i<constraints.size(); ++i){
        (uaiger->constraints).push_back(value(constraints[i]));
    }
    //deal with output
    int newbad = value(allbad[propertyIndex]);
    uaiger->outputs.push_back(newbad);       //uaiger->outputs.push_back(value(bad));
    uaiger->nodes[abs(newbad)].fathers++;    //uaiger->nodes[abs(value(bad))].fathers++;
    
    //deal with register
    if(unfold_latches) cout << "unfold latches" << endl;
    for(int i=0; i<=num_latches-1; ++i){
        tempvalue[i] = value(nexts[i]);   
    }
    for(int i=0; i<=num_latches-1; ++i){
        values[i+num_inputs+2] = tempvalue[i];
        if(unfold_latches) cout << nexts[i] << "-> " << values[i+num_inputs+2] << endl;  
    }  
}

void BMC::initialize(){
    //show_bads();

    //show_constraints();

    //show_variables();
    
    //show_ands();

    //show_nexts();

    //cout << "start BMC initialize" <<endl;

    //check init
    bmcSolver = new CaDiCaL();
    encode_init_condition(bmcSolver);
    bmcSolver = nullptr;

    //for unfold
    uaiger = new UnfoldAiger;
    (uaiger->hash_table).resize(99999999);
    memset(tempvalue,0,sizeof(tempvalue));
    bmc_frame_k = 0;

    //for solve
    bmcSolver = new CaDiCaL();
    ipasir_set_terminate(bmcSolver->s, nullptr, terminate_callback);
    bmcSolver->add(-1); bmcSolver->add(0); 
    lit_has_insert.resize(99999999);       //lit_has_insert.resize((uaiger->ands).size()+2); 
    bmc_frame_k = 0;

    //unfold init
    //cout << "start BMC unfold" <<endl;
    uaiger->nodes.push_back(Node(0, 0, 0, 0));  // string("NULL")
    uaiger->nodes.push_back(Node(1, 0, 0, 0));  // string("False")
    //deal with init latches (init = false = x1, or init = true = -x1)
    values.resize(variables.size());
    values[1] = 1;                         // x1 = false                  
    for(int latch: init_state){
        if(latch > 0) values[latch] = -1;  // values[latch] = -x1 = true
            else if(latch < 0) values[-latch] = 1;
    }
    for(int i=0; i<=num_latches-1; ++i){
        uaiger->nodes.push_back(Node(2, 0, 0, 0)); // satrt from the 3rd node of uaiger
        if(values[i+num_inputs+2] == 0){
            values[i+num_inputs+2] = uaiger->nsize()-1;       
        } 
    }
}

//check all frames
int BMC::check(){
    initialize();
    int res;
    for(bmc_frame_k = 1; bmc_frame_k <= nframes; bmc_frame_k++){
        if(RESULT!=0) return 0; 
        unfold();
        res = solve_one_frame();
        if (res == 10) {
            std::lock_guard<std::mutex> lock(result_mutex);
            if(RESULT == 0){
                RESULT = 10;
                if(moreinfo_for_bmc) uaiger->show_statistics();
                if(moreinfo_for_bmc) cout << "Output was asserted in frame." << endl;
                if(output_witness){
                    cout << "1\n";
                    cout << "b0\n";
                    vector<char> a(num_inputs + num_latches + 2, 'x');
                    for(int latch: init_state)
                        a[abs(latch)] = (latch>0?'1':'0');
                    for(int i=0; i<num_latches; ++i){
                        int latch_index = unprimed_first_dimacs + num_inputs + i;
                        //cout << latch_index << " ";
                        if(a[latch_index] == 'x'){
                            int assignment = bmcSolver->val(unprimed_first_dimacs + i);
                            if(assignment != 0)
                                a[latch_index] = (assignment<0?'0':'1');
                        }
                        //cout << a[latch_index] << " ";
                        cout << a[latch_index];
                    }
                    cout << endl;
                    int inputCount = 0;
                    for(int i : uaiger->inputs){
                        int assignment = bmcSolver->val(i);
                        if(assignment != 0)
                            cout << (assignment<0?'0':'1');
                        else 
                            cout << 'x';
                        inputCount++;
                        if(inputCount % num_inputs == 0)  cout << '\n';
                    }
                    cout << ".\n";    
                }   
            }
            return 10;
        } 
        if(res == 0 || abs((uaiger->outputs).back()) > max_nodes) 
            return 0; // bmcsolver terminate
    } 
    return 0; // safe in frames
}

// check one frame
int BMC::solve_one_frame(){
    set<int> lit_set;
    int bad = (uaiger->outputs).back();
    lit_set.insert(abs(bad));
    
    int cons_count = (uaiger->constraints).size();
    for(int i=cons_count - constraints.size(); i<cons_count; ++i){
        lit_set.insert(abs(uaiger->constraints[i]));
        bmcSolver->add(uaiger->constraints[i]); bmcSolver->add(0);
    }

    while(lit_has_insert.size() < (uaiger->ands).size()){
        lit_has_insert.push_back(0);
    }
    while((uaiger->hash_table).size() < (uaiger->nodes).size() + num_ands * 3){
        (uaiger->hash_table).push_back(vector<And>());
    }
    
    bool use_coi = 1;
    int lowbound = 0;
    if(use_coi == 0) lowbound = max_index_of_ands_added_to_solver + 1;

    for(int i = (uaiger->ands).size()-1; i>=lowbound; i--){   
        And a = uaiger->ands[i]; 

        //judge if the AND gate is added before, or use coi 
        if(use_coi){
            if(lit_has_insert[i] == 1 || lit_set.find(a.o) == lit_set.end()){
                continue;
            }
        }

        lit_set.erase(a.o);        
        lit_has_insert[i] = 1;
        lit_set.insert(abs(a.i1));
        lit_set.insert(abs(a.i2));
        
        bmcSolver->add(-a.o); bmcSolver->add(a.i1);  bmcSolver->add(0);
        bmcSolver->add(-a.o); bmcSolver->add(a.i2);  bmcSolver->add(0);
        bmcSolver->add(a.o);  bmcSolver->add(-a.i1); bmcSolver->add(-a.i2); bmcSolver->add(0);
    }
    max_index_of_ands_added_to_solver = (uaiger->ands).size()-1;

    if(bmc_frame_k % max_thread_index != (thread_index-1)) {
        bmcSolver->add(-bad); bmcSolver->add(0); 
        return 20;
    }

    // skip steps
    // if(bmc_frame_k < 100){
    //     bmcSolver->add(-bad); bmcSolver->add(0); 
    //     if(bmc_frame_k < 10 || bmc_frame_k % 20 == 0) cout << "frames = "<< bmc_frame_k <<", bad = " << bad << ", skip" << endl;
    //     return 20;
    // }

    bmcSolver->assume(bad);
    int result = bmcSolver->solve();
    if(result == 20){
        bmcSolver->add(-bad); bmcSolver->add(0); 
        if(moreinfo_for_bmc) cout << "frames = "<< bmc_frame_k <<", bad = " << bad << ", res = " << result << endl;
        return 20;
    } 
    else if(result == 10){
        if(moreinfo_for_bmc) cout << "frames = "<< bmc_frame_k <<", bad = " << bad << ", res = " << result << endl;
        return 10;
    }
    else{
        return 0;  // bmcsolver terminate
    }
}