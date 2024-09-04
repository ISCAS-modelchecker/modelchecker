#include "PDR.hpp"
#include "sat_solver.hpp"
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <queue>
#include <boost/lockfree/spsc_queue.hpp>
#include <mutex>

std::mutex result_mutex;  // 定义一个互斥锁
unsigned long long state_count = 0;
int RESULT = 0; // 0 means safe in PORTFOLIO; 10 means find a bug; 20 proves safety
boost::lockfree::spsc_queue<share_Cube, boost::lockfree::capacity<500000>> cube_producer[2][8]; //两个主线程 共享 八个子线程的子句
queue<share_Cube> cube_register[2][9999];

//  Log functions
// --------------------------------------------
void PDR::show_state(State *s){
    vector<char> a(nInputs + nLatches + 2, 'x');
    for(int i : s->inputs)
        a[abs(i)] = (i<0?'0':'1');
    for(int l : s->latches)
        a[abs(l)] = (l<0?'0':'1');

    cout<<'[';
    for(int i=1; i<=nInputs; ++i)
        cout<<a[1+i];
    cout<<'|';
    for(int l=1; l<=nLatches; ++l)
        cout<<a[1+nInputs+l];
    cout<<']';
    cout<<endl;
}

string PDR::return_state(State *s){
    vector<char> a(nInputs + nLatches + 2, 'x');
    for(int i : s->inputs)
        a[abs(i)] = (i<0?'0':'1');
    for(int l : s->latches)
        a[abs(l)] = (l<0?'0':'1');

    string str="";
    str.push_back('[');
    for(int i=1; i<=nInputs; ++i)
        str.push_back(a[1+i]);
    str.push_back('|');
    for(int l=1; l<=nLatches; ++l)
        str.push_back(a[1+nInputs+l]);
    str.push_back(']');
    return str;
}

string PDR::return_input(State *s){
    vector<char> a(nInputs + nLatches + 2, 'x');
    for(int i : s->inputs)
        a[abs(i)] = (i<0?'0':'1');
    for(int l : s->latches)
        a[abs(l)] = (l<0?'0':'1');
    string str="";
    for(int i=1; i<=nInputs; ++i)
        str.push_back(a[1+i]);
    return str;
}

string PDR::return_latch(State *s){
    vector<char> a(nInputs + nLatches + 2, 'x');
    for(int i : s->inputs)
        a[abs(i)] = (i<0?'0':'1');
    for(int l : s->latches)
        a[abs(l)] = (l<0?'0':'1');
    string str="";
    for(int l=1; l<=nLatches; ++l)
        str.push_back(a[1+nInputs+l]);
    return str;
}

void PDR::show_lit(int l) const{
    cout<<(l<0?"-":"")<<variables[abs(l)].name;
}

void PDR::show_litvec(vector<int> &lv) const{
    for(int l:lv){
        show_lit(l);
        cout<<" ";
    }cout<<endl;
}

string PDR::return_litvec(vector<int> &lv) const{
    string str = "";
    for(int l:lv){
        str.append(l<0?"-":"");
        str.append(variables[abs(l)].name);
        str.push_back(' ');
    }
    return str;
}

double PDR::get_runtime(){
    auto stop_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop_time - start_time;
    return elapsed_seconds.count();
}

void PDR::log_witness(){
    State* p = cex_state_idx;
    set<State *> ss;
    while(p != nullptr){
        cex_states.push_back(p);
        ss.insert(p);
        p = p->next;
    }
    for(auto q : states){
        if(ss.find(q) == ss.end()){
            delete q;
        }
    }
}

void PDR::show_witness(){
    bool need_delete = true;
    assert(find_cex);
    if(!no_output) cout <<"AIGER witness:\n";
    cout << "1\n";
    cout << "b0\n";
    int initial_state = 1;
    for(auto s : cex_states){
        if(initial_state){
            cout << return_latch(s) + "\n";
            initial_state = 0;
        }
        cout << return_input(s) + "\n";
    }
    cout << ".\n";
    //cout<<"c CEX witness:\n";
    for(auto s : cex_states){
        if(!no_output) cout << return_state(s) + "\n";
        if(need_delete){
            delete s;
        }
    }
}

void PDR::show_variables(){
    int ct = 1;
    for(int i=1; i<variables.size(); ++i){
        Variable &v = variables[i];
        // assert(i == v.dimacs_var);
        cout << "v" << v.dimacs_var << "(" << v.name << ")    \t";
        if(ct++ % 20 == 0 || (i+1<variables.size() && variables[i+1].name[0] != v.name[0])){
            cout << endl;
            ct = 1;
        }
    }
    cout << endl;
}

void PDR::show_PO(){
    cout<<" + -----"<<endl;

    for(Obligation o : obligation_queue){
        cout<<"L"<<o.frame_k<<" d"<<o.depth<<" S: ";
        show_litvec(o.state->latches);
        // show_state(o.state);
    }

    cout<<" + ====="<<endl;
}

void PDR::show_frames(){
    int k=0, safe=0;
    cout<<endl;
    for(Frame & f: frames){
        cout << "Level " << k++ <<" (cubeCount = " << f.cubes.size();
        cout<<" varCount =  "<<f.solver->max_var()<<" ";
        cout << ") :" << endl;
        if(k == 1) continue;
        if(f.cubes.size() == 0) {
            safe=1;continue;
        }
        if(safe==0) continue;
        
        for(const Cube &c : f.cubes){
            Cube cc = c;
            show_litvec(cc);
        }
        safe=0;
    }
    cout<<"\n * ======="<<endl;
}

void PDR::show_aag(){
    std::ofstream file("certificate.aag");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << "certificate.aag" << " for writing." << std::endl;
        return;
    }    
    // 输出AAG格式的头部
    file << "aag " << (nInputs + nLatches + nAnds) << " " << nInputs << " " << nLatches << " 0 " << nAnds << " 1";
    if(constraints.size()>0){
        file << " " << constraints.size();
    }
    file << std::endl;
    // 输出输入变量
    for(int i = 1; i <= nInputs; ++i) {
        file << 2 * i << std::endl;
    }
    // 输出寄存器（latches）
    for(int i = 1; i <= nLatches; ++i) {
        file << 2 * (nInputs + i) << " " << dimacs_to_aiger(nexts[i-1]);
        if(aiger->latches[i-1].default_val != 0) {
            file << " " << aiger->latches[i-1].default_val;
        }
        file << std::endl;
    }
    // 输出 bad 信号
    file << dimacs_to_aiger(bad) << std::endl;
    // 输出约束
    for(int i = 0; i < constraints.size(); ++i) {
        file << dimacs_to_aiger(constraints[i]) << std::endl;
    }
    // 输出 AND 门
    for(int i = 0; i < ands.size(); ++i) {
        file << dimacs_to_aiger(ands[i].o) << " ";
        file << dimacs_to_aiger(ands[i].i1) << " ";
        file << dimacs_to_aiger(ands[i].i2) << std::endl;
    }
    // 关闭文件
    file.close(); 
    // 调用aigtoaig工具将生成的AAG文件转换为AIG文件
    //if(aiger->binaryMode){
    std::string command = "./aigtoaig certificate.aag certificate.aig";
    int result = system(command.c_str());
    //}

    // printf("aag %d %d %d 0 %d 1", nInputs + nLatches + nAnds, nInputs, nLatches, nAnds);
    // if(constraints.size()>0){
    //     cout<<" "<< constraints.size();
    // }cout<<endl;

    // for(int i=1; i<=nInputs; ++i)
    //     cout<<2*i<<endl;
    // for(int i=1; i<=nLatches; ++i){
    //     cout<<2*(nInputs+i)<<" "<<dimacs_to_aiger(nexts[i-1]);
    //     if(aiger->latches[i-1].default_val != 0)
    //         cout<<" "<<aiger->latches[i-1].default_val;
    //     cout<<endl;
    // }
    
    // cout<<dimacs_to_aiger(bad)<<endl;
    // for(int i=0; i<constraints.size(); ++i)
    //     cout<<dimacs_to_aiger(constraints[i])<<endl;

    // for(int i=0; i<ands.size(); ++i){
    //     cout<<dimacs_to_aiger(ands[i].o)<<" ";
    //     cout<<dimacs_to_aiger(ands[i].i1)<<" ";
    //     cout<<dimacs_to_aiger(ands[i].i2);
    //     cout<<endl;
    // }
}

void PDR::show_aig(){
    printf("aig %d %d %d 0 %d 1", nInputs + nLatches + nAnds, nInputs, nLatches, nAnds);
    if(constraints.size()>0){
        cout<<" "<< constraints.size();
    }cout<<endl;

    for(int i=1; i<=nLatches; ++i){
        cout<<dimacs_to_aiger(nexts[i-1]);
        if(aiger->latches[i-1].default_val != 0)
            cout<<" "<<aiger->latches[i-1].default_val;
        cout<<endl;
    }

    cout<<dimacs_to_aiger(bad)<<endl;
    for(int i=0; i<constraints.size(); ++i)
        cout<<dimacs_to_aiger(constraints[i])<<endl;

    string str;
    for(int i=0; i<ands.size(); ++i){
        int d1 = dimacs_to_aiger(ands[i].o) - dimacs_to_aiger(ands[i].i1);
        cout<<dimacs_to_aiger(ands[i].i1) - dimacs_to_aiger(ands[i].i2);
        cout<<endl;
    }
    cout << str << endl;
}

int PDR::prime_var(int var){
    assert(var >= 1);
    if(var > 1){
        if(var <= 1 + nInputs + nLatches){
            return primed_first_dimacs + var - 2;
        }else{
            if(map_to_prime.find(var) == map_to_prime.end()){
                int unprimed_var = var;
                int primed_var = variables.size();
                map_to_prime[unprimed_var] = primed_var;
                map_to_unprime[primed_var] = unprimed_var;
                string new_name = variables[unprimed_var].name + string("'");
                variables.push_back(Variable(primed_var, new_name));
                return primed_var;
            }else{
                return map_to_prime[var];
            }
        }
    }else{
        // use const True/False
        return var;
    }
}

int PDR::prime_lit(int lit){
    if(lit >= 0){
        return prime_var(lit);
    }else{
        return -prime_var(-lit);
    }
}

void PDR::simplify_aiger(){
    //TODO: rewriting aiger
}

// translate the aiger language to internal states
void PDR::translate_to_dimacs(){
    variables.clear();
    ands.clear();
    nexts.clear();
    init_state.clear();
    constraints.clear();
    constraints_prime.clear();
    map_to_prime.clear();
    map_to_unprime.clear();

    variables.push_back(Variable(0, string("NULL")));
    variables.push_back(Variable(1, string("False")));

    // load inputs
    nInputs = aiger->num_inputs;
    for(int i=1; i<=nInputs; ++i){
        assert((i)*2 == aiger->inputs[i-1]);
        variables.push_back(Variable(1 + i, 'i', i-1, false));
    }

    // load latches
    nLatches = aiger->num_latches;
    for(int i=1; i<=nLatches; ++i){
        assert((nInputs+i)*2 == aiger->latches[i-1].l);
        variables.push_back(Variable(1 + nInputs + i, 'l', i-1, false));
    }

    // load ands
    nAnds = aiger->num_ands;
    for(int i=1; i<=nAnds; ++i){
        assert(2*(nInputs+nLatches+i) == aiger->ands[i-1].o);
        int o = 1+nInputs+nLatches+i;
        int i1 = aiger_to_dimacs(aiger->ands[i-1].i1);
        int i2 = aiger_to_dimacs(aiger->ands[i-1].i2);
        variables.push_back(Variable(o, 'a', i-1, false));
        ands.push_back(And(o, i1, i2));
    }

    // deal with initial states
    for(int i=1; i<=nLatches; ++i){
        int l = 1 + nInputs + i;
        assert((l-1)*2 == aiger->latches[i-1].l);
        Aiger_latches & al = aiger->latches[i-1];
        nexts.push_back(aiger_to_dimacs(al.next));
        if(al.default_val==0){
            init_state.push_back(-l);
        }else if(al.default_val==1){
            init_state.push_back(l);
        }
    }

    // deal with constraints
    for(int i=0; i<aiger->num_constraints; ++i){
        int cst = aiger->constraints[i];
        constraints.push_back(aiger_to_dimacs(cst));
    }

    // load bad states
    if(aiger->num_bads > 0 && aiger->num_bads > property_index){
        int b = aiger->bads[property_index];
        bad = aiger_to_dimacs(b);
    }else if(aiger->num_outputs > 0 && aiger->num_outputs > property_index){
        int output = aiger->outputs[property_index];
        bad = aiger_to_dimacs(output);
    }else{
        assert(false);
    }
    assert(abs(bad) <= variables.size());

    // load inputs prime
    primed_first_dimacs = variables.size();
    assert(primed_first_dimacs == 1 + nInputs + nLatches + nAnds + 1);
    for(int i=0; i<nInputs; ++i){
        variables.push_back(
            Variable(primed_first_dimacs + i, 'i', i, true));
    }

    // load latches prime
    for(int i=0; i<nLatches; ++i){
        variables.push_back(
            Variable(primed_first_dimacs + nInputs + i, 'l', i, true));
    }

    // load bad prime
    bad_prime = prime_lit(bad);

    // load constraint prime
    for(int i=0; i<constraints.size(); ++i){
        int pl = prime_lit(constraints[i]);
        constraints_prime.push_back(pl);
    }
    if (aig_veb){
        for(int i=1; i<=nLatches; ++i){
            Aiger_latches & al = aiger->latches[i-1];
            cout << 1 + nInputs + i << " " << nexts[i-1] << " " << al.default_val << endl;
        }
        cout << "bad: " << bad << endl;
        cout << "primed bad: " << bad_prime << endl;
        cout << "constraints: \n";
        for(int i=0; i<aiger->num_constraints; ++i) cout << constraints[i] << " ";
        cout << "\nprimed constraints: \n";
        for(int i=0; i<aiger->num_constraints; ++i) cout << constraints_prime[i] << " ";
        cout << endl;
        for(int i=0; i<ands.size(); ++i){
            cout<<ands[i].o<<" ";
            cout<<ands[i].i1<<" ";
            cout<<ands[i].i2;
            cout<<endl;
        }
    }

    if(aig_veb > 2)
        show_variables();
}

void PDR::initialize_heuristic(){
    heuristic_lit_cmp = new heuristic_Lit_CMP();
    heuristic_lit_cmp->counts.clear();
    (heuristic_lit_cmp->counts).resize(nInputs+nLatches+nInputs+2);
    //对应 unprimed_first_dimacs + 0 to unprimed_first_dimacs + nInputs - 1
    for(int i = 2; i <= nInputs+1; i+=10){
        heuristic_lit_cmp->counts[i] = 0.5;
    }
    //对应 unprimed_first_dimacs + nInputs to unprimed_first_dimacs + + nInputs + nLatches - 1
    for(int i = nInputs+2; i <= nInputs+nLatches+1; i+=10){
        heuristic_lit_cmp->counts[i] = 0.5;
    }
    //对应 primed_first_dimacs + 0 to primed_first_dimacs + nInputs - 1
    // for(int i = nInputs+nLatches+2; i <= nInputs+nLatches+nInputs+1; i+=10){
    //     heuristic_lit_cmp->counts[i] = 0.5;
    // }
}

void PDR::updateLitOrder(Cube &cube, int level){
    //decay
    for(int i = 0; i < nInputs+nLatches+nInputs+2; i++){
        heuristic_lit_cmp->counts[i] *= 0.99;
    }
    //update heuristic score of literals remove abs
    for(int index: cube){
        heuristic_lit_cmp->counts[abs(index)] += 10.0/cube.size();
    }
    //check
    if(output_stats_for_heuristic){
        for(int i=0; i<heuristic_lit_cmp->counts.size(); i++){
            cout << heuristic_lit_cmp->counts[i] << " ";
        }
        cout << endl;
    }
}

void PDR::initialize(){
    satelite = nullptr;
    simplify_aiger();
    translate_to_dimacs();
    initialize_heuristic();

    nQuery = nCTI = nCTG = nmic = nCoreReduced = nAbortJoin = nAbortMic = 0;
    if(!no_output) cout<<"c PDR constructed from aiger file [Finished] \n";
}

void PDR::new_frame(){
    int last = frames.size();
    frames.push_back(Frame());
    encode_translation(frames[last].solver);
    // Goal of frame-k is to find a pre-condition or prove clause c inductive to k-1.
    for(int l : constraints_prime){
        frames[last].solver->add(l);
        frames[last].solver->add(0);
    }
}

int add_ct = 0;
void PDR::add_cube(Cube &cube, int k, bool to_all, bool ispropagate, int isigoodlemma){
    if(!ispropagate)
        earliest_strengthened_frame =min(earliest_strengthened_frame,k);
    sort(cube.begin(), cube.end(), Lit_CMP());
    pair<set<Cube, Cube_CMP>::iterator, bool> rv = frames[k].cubes.insert(cube);
    if(!rv.second) return;

    if(share_memory and main_thread_index < 0) { //and k and cube.size()<5 and isigoodlemma and
        if(k>9990) k=9990;
        cube_producer[0][thread_index].push(share_Cube(to_all, k, cube));
        cube_producer[1][thread_index].push(share_Cube(to_all, k, cube));
        //cout << "thread" + to_string(thread_index) + " enqueue " + return_litvec(cube) +"\n";
    }

    if(output_stats_for_addcube and !ispropagate) {
        cout<<"add Cube: (sz"<<cube.size()<<") to "<<k<<" : ";
        show_litvec(cube);
    }
    if(to_all){
        for(int i=1; i< k; ++i){
            for(int l : cube)
                frames[i].solver->add(-l);
            frames[i].solver->add(0);
        }
    }
    for(int l : cube)
        frames[k].solver->add(-l);
    frames[k].solver->add(0);

    // update heuristics igoodlemma
    if(use_heuristic and use_heuristic_igoodlemma and (thread_index == 1 || thread_index == 2)){
        while(isigoodlemma){
            updateLitOrder(cube, k);
            isigoodlemma--;
        }
    }
    else if(use_heuristic and !ispropagate)
        updateLitOrder(cube, k);
}

bool PDR::is_init(vector<int> &latches){
    if(init == nullptr){
        init = new CaDiCaL();
        // init = new minisatCore();
        encode_init_condition(init);
    }
    for(int l : latches)
        init->assume(l);
    int res = init->solve();
    assert(res != 0);
    return (res == SAT);
}

// check ~latches inductive by Fi /\ -s /\ T /\ s'
// Fi /\ -latches /\ [constraints /\ -bad /\ T] /\ constraints' /\ latches'
int core_ct = 0;
bool PDR::is_inductive(SATSolver *solver, const Cube &latches, bool gen_core, bool reverse_assumption){
    solver->clear_act();
    vector<int> assumptions;
    int act = solver->max_var() + 1;
    solver->add(-act);
    for(int i : latches)
        solver->add(-i);
    solver->add(0);

    if(use_heuristic){
        for(int i : latches)
            assumptions.push_back(i);
        stable_sort(assumptions.begin(), assumptions.end(), *heuristic_lit_cmp);
        if(reverse_assumption)
            reverse(assumptions.begin(), assumptions.end());
        for(int i=0; i<assumptions.size(); i++){
            assumptions[i] = prime_lit(assumptions[i]);
        }
    }
    else{
        for(int i : latches)
            assumptions.push_back(prime_lit(i));
        stable_sort(assumptions.begin(), assumptions.end(), Lit_CMP());
    }

    solver->assume(act);
    for(int i: assumptions)
        solver->assume(i);

    int status = solver->solve();
    ++nQuery;
    assert(status != 0);
    bool res = (status == UNSAT);

    if(res & gen_core){
        core.clear();
        for(int i : latches){
            if(solver->failed(prime_lit(i)))
                core.push_back(i);
        }
        if(is_init(core)){
            core = latches;
        }
    }
    solver->set_clear_act();
    return res;
}

void PDR::encode_init_condition(SATSolver *s){
    s->add(-1); s->add(0);
    for(int l : init_state){
        s->add(l); s->add(0);
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
    if(aig_veb > 2)
        cout<<"c add_cls finish load init"<<endl;
}

void PDR::encode_bad_state(SATSolver *s){
    set<int> lit_set;
    lit_set.insert(abs(bad));
    for(auto i = ands.rbegin(); i != ands.rend(); ++i){
        And & a = *i;
        assert(a.o > 0);
        if(lit_set.find(a.o) == lit_set.end())
            continue;
        lit_set.insert(abs(a.i1));
        lit_set.insert(abs(a.i2));
        s->add(-a.o); s->add(a.i1);  s->add(0);
        s->add(-a.o); s->add(a.i2);  s->add(0);
        s->add(a.o);  s->add(-a.i1); s->add(-a.i2); s->add(0);
    }
    if(aig_veb > 2)
        cout<<"c add_cls finish load bad"<<endl;
}

// check ~latches inductive by Fi /\ -s /\ T /\ s'
// Fi /\ latches /\ [constraints /\ -bad /\ T] /\ constraints' /\ latches'
// lift a pre-state by Fi /\ pre /\ T /\ -s' (assert UNSAT)
// Fi /\ inputs /\ latches /\ [constraints /\ -bad /\ T] /\ inputs' /\ -(constraints' /\ latches')
// translation encoding: [constraints /\ -bad /\ T]
void PDR::encode_translation(SATSolver *s, bool cons){
    if(satelite == nullptr){
        satelite = new minisatSimp();
        satelite->var_enlarge_to(variables.size()-1);

        for(int i=1; i<= nInputs+nLatches; ++i){
            satelite->set_frozen(1 + i);
            satelite->set_frozen(prime_var(1 + i));
        }
        satelite->set_frozen(abs(bad));
        satelite->set_frozen(abs(bad_prime));
        for(int i=0; i<constraints.size(); ++i){
            assert(prime_var(abs(constraints[i])) == abs(constraints_prime[i]));
            satelite->set_frozen(abs(constraints[i]));
            satelite->set_frozen(abs(constraints_prime[i]));
        }

        set<int> prime_lit_set;
        prime_lit_set.insert(abs(bad));
        for(int l : constraints)
            prime_lit_set.insert(abs(l));

        set<int> lit_set(prime_lit_set.begin(), prime_lit_set.end());
        for(int l : nexts)
            lit_set.insert(abs(l));

        satelite->add(-1); satelite->add(0);    // literal 1 is const 'T'
        satelite->add(-bad); satelite->add(0);  // -bad must hold !
        if(cons){
            for(int l : constraints){
                if(l == bad) satelite_unsat = true;
                satelite->add(l);satelite->add(0);
            }
        }
        //for(int l : constraints_prime){satelite->add(l); satelite->add(0);}
        for(int i=0; i<nLatches; ++i){
            int l = 1 + nInputs + i + 1;
            int pl = prime_lit(l);
            int next = nexts[i];
            satelite->add(-pl);satelite->add(next); satelite->add(0);
            satelite->add(-next); satelite->add(pl); satelite->add(0);
        }

        for(auto i = ands.rbegin(); i != ands.rend(); ++i){
            And &a = *i;
            assert(a.o > 0);
            if(lit_set.find(a.o) != lit_set.end()){
                lit_set.insert(abs(a.i1));
                lit_set.insert(abs(a.i2));

                satelite->add(-a.o); satelite->add(a.i1);  satelite->add(0);
                satelite->add(-a.o); satelite->add(a.i2);  satelite->add(0);
                satelite->add(a.o);  satelite->add(-a.i1); satelite->add(-a.i2); satelite->add(0);
                //cout << "andgate " << a.o <<  " " << a.i1 << " " << a.i2 << endl;

                if(prime_lit_set.find(a.o) != prime_lit_set.end()){
                    int po  = prime_lit(a.o);
                    int pi1 = prime_lit(a.i1);
                    int pi2 = prime_lit(a.i2);

                    prime_lit_set.insert(abs(a.i1));
                    prime_lit_set.insert(abs(a.i2));
                    satelite->add(-po); satelite->add(pi1);  satelite->add(0);
                    satelite->add(-po); satelite->add(pi2);  satelite->add(0);
                    satelite->add(po);  satelite->add(-pi1); satelite->add(-pi2); satelite->add(0);
                    //cout << "andgate " << po <<  " " << pi1 << " " << pi2 << endl;
                }
            }
        }
        satelite->simplify();
        // for(int l : satelite->simplified_cnf){
        //     cout << l << " ";
        //     if(l == 0){
        //         cout << endl;
        //     }
        // }
    }

    for(int l : satelite->simplified_cnf){
         s->add(l);
    }
    if(satelite_unsat) {s->add(1);s->add(0);}
    if(aig_veb > 2)
        cout<<"c add_cls finish load translation"<<endl;
}

void PDR::encode_lift(SATSolver *s){
    if(satelite2 == nullptr){
        satelite2 = new minisatSimp();
        satelite2->var_enlarge_to(variables.size()-1);

        for(int i=1; i<= nInputs+nLatches; ++i){
            satelite2->set_frozen(1 + i);
            satelite2->set_frozen(prime_var(1 + i));
        }
        satelite2->set_frozen(abs(bad));
        satelite2->set_frozen(abs(bad_prime));
        for(int i=0; i<constraints.size(); ++i){
            assert(prime_var(abs(constraints[i])) == abs(constraints_prime[i]));
            satelite2->set_frozen(abs(constraints[i]));
            satelite2->set_frozen(abs(constraints_prime[i]));
        }

        set<int> prime_lit_set;
        prime_lit_set.insert(abs(bad));
        for(int l : constraints)
            prime_lit_set.insert(abs(l));

        set<int> lit_set(prime_lit_set.begin(), prime_lit_set.end());
        for(int l : nexts)
            lit_set.insert(abs(l));

        satelite2->add(-1); satelite2->add(0);    // literal 1 is const 'T'
        satelite2->add(-bad); satelite2->add(0);  // -bad must hold !

        for(int i=0; i<nLatches; ++i){
            int l = 1 + nInputs + i + 1;
            int pl = prime_lit(l);
            int next = nexts[i];
            satelite2->add(-pl);satelite2->add(next); satelite2->add(0);
            satelite2->add(-next); satelite2->add(pl); satelite2->add(0);
        }

        for(auto i = ands.rbegin(); i != ands.rend(); ++i){
            And &a = *i;
            assert(a.o > 0);

            if(lit_set.find(a.o) != lit_set.end()){
                lit_set.insert(abs(a.i1));
                lit_set.insert(abs(a.i2));

                satelite2->add(-a.o); satelite2->add(a.i1);  satelite2->add(0);
                satelite2->add(-a.o); satelite2->add(a.i2);  satelite2->add(0);
                satelite2->add(a.o);  satelite2->add(-a.i1); satelite2->add(-a.i2); satelite2->add(0);

                if(prime_lit_set.find(a.o) != prime_lit_set.end()){
                    int po  = prime_lit(a.o);
                    int pi1 = prime_lit(a.i1);
                    int pi2 = prime_lit(a.i2);

                    prime_lit_set.insert(abs(a.i1));
                    prime_lit_set.insert(abs(a.i2));
                    satelite2->add(-po); satelite2->add(pi1);  satelite2->add(0);
                    satelite2->add(-po); satelite2->add(pi2);  satelite2->add(0);
                    satelite2->add(po);  satelite2->add(-pi1); satelite2->add(-pi2); satelite2->add(0);
                }
            }
        }
        if(split_ands){
            notInvConstraints = satelite2->max_var() + 1;
            satelite2->set_frozen(notInvConstraints);
            satelite2->add(-notInvConstraints);
            for(int l : constraints_prime)
                satelite2->add(-l);
            for(int l : constraints)
                satelite2->add(-l);
            satelite2->add(0);
        }
        satelite2->simplify();
    }

    for(int l : satelite2->simplified_cnf){
         s->add(l);
    }
}

// Lifts the pre-state on a given frame
// If SAT?[Fk /\ -succ /\ T /\ succ'] return SAT and extract a pre-state pre \in [Fk /\ -succ]
// Then call SAT?[pre /\ T /\ -succ']
// pre must meet constraints.
void PDR::extract_state_from_sat(SATSolver *sat, State *s, State *succ){
    s->clear();
    if(lift == nullptr){
        lift = new CaDiCaL();
        //lift = new minisatCore();
        encode_lift(lift);
    }

    lift->clear_act();
    vector<int> assumptions, latches;
    int distance = primed_first_dimacs - (nInputs+nLatches+2);
    // inputs /\ inputs' /\ pre
    for(int i=0; i<nInputs; ++i){
        int ipt = sat->val(unprimed_first_dimacs + i);
        int pipt = sat->val(primed_first_dimacs + i);
        if(ipt != 0){
            s->inputs.push_back(ipt);
            assumptions.push_back(ipt);
        }
        if(pipt > 0){
            pipt = (pipt-distance);
            assumptions.push_back(pipt);
        }
        else if (pipt < 0){
            pipt = -(-pipt-distance);
            assumptions.push_back(pipt);
        }
    }
    int sz = assumptions.size();

    for(int i=0; i<nLatches; ++i){
        int l = sat->val(unprimed_first_dimacs + nInputs + i);
        if(l!=0){
            latches.push_back(l);
            assumptions.push_back(l);
        }
    }

    // encoding -(constraints' /\ pre') <-> -constraints' \/ -pre'
    int act_var = lift->max_var() + 1;
    lift->add(-act_var);
    if(split_ands)
        lift->add(notInvConstraints);
    else{
        for(int l : constraints)
            lift->add(-l);
        for(int l : constraints_prime)
            lift->add(-l);
    }
    if(succ == nullptr)
        lift->add(-bad_prime);
    else{
        for(int l: succ->latches)
            lift->add(prime_lit(-l));
    }
    lift->add(0);

    if(use_heuristic and assumption_allsort){
        stable_sort(assumptions.begin(), assumptions.end(), *heuristic_lit_cmp);
        reverse(assumptions.begin(), assumptions.end());
    }
    else if(use_heuristic){
        stable_sort(assumptions.begin()+sz, assumptions.end(), *heuristic_lit_cmp);
        reverse(assumptions.begin()+sz, assumptions.end());
    }
    else
        stable_sort(assumptions.begin(), assumptions.end(), Lit_CMP());

    for(int i=0; i<assumptions.size(); i++){
        if(assumptions[i] >= nInputs+nLatches+2)
            assumptions[i] = assumptions[i] + distance;
        else if(assumptions[i] <= -(nInputs+nLatches+2))
            assumptions[i] = assumptions[i] - distance;
    }

    lift->assume(act_var);
    for(int l : assumptions)
        lift->assume(l);
    int res = lift->solve();
    ++nQuery;

    assert(res == UNSAT);

    if(output_stats_for_extract){
        cout << "assumptions ";
        for(int l : assumptions){
            if(abs(l) < primed_first_dimacs)
                cout << variables[abs(l)].name << " "<< heuristic_lit_cmp->counts[abs(l)] << " ";
            else
                cout << variables[abs(l)].name << " "<< heuristic_lit_cmp->counts[abs(l)-distance] << " ";
        }
        cout << endl;

        vector<char> a(nInputs + nLatches + 2, 'x');
        for(int i : s->inputs)
            a[abs(i)] = (i<0?'0':'1');
        for(int l : latches)
            a[abs(l)] = (l<0?'0':'1');

        cout<<'[';
        for(int i=1; i<=nInputs; ++i)
            cout<<a[1+i];
        cout<<'|';
        for(int l=1; l<=nLatches; ++l)
            cout<<a[1+nInputs+l];
        cout<<']';
        cout<<endl;

        cout << "lift \n";
        for(int l : assumptions){
            if(lift->failed(l))
                cout << variables[abs(l)].name << " ";
        }
        cout << endl;
    }

    // int last_index = 0, corelen = 0;
    // if(true){
    //     int core_literal = 0;
    //     for(int i=0; i<assumptions.size(); i++){
    //         int l = assumptions[i];
    //         if(abs(l) >= nInputs+2 and abs(l) <= nInputs+nLatches+1) corelen++;
    //         if(lift->failed(l)){
    //             double score = (i-core_literal)/20.0;
    //             if(score > 1.0) score = 1.05;
    //             if(abs(l) < primed_first_dimacs)
    //                 heuristic_lit_cmp->counts[abs(l)] += score; //score
    //             else
    //                 heuristic_lit_cmp->counts[abs(l)-distance] += score;
    //             core_literal = i;
    //             last_index = corelen;
    //         }
    //     }
    // }

    for(int l : latches){
        if(lift->failed(l)){
            s->latches.push_back(l);
        }
    }

    s->next = succ;
    lift->set_clear_act();
}

// solving SAT?[ Fk /\ T /\ -Bad' ]
bool PDR::get_pre_of_bad(State *s){
    s->clear();
    int Fk = depth();
    frames[Fk].solver->assume(bad_prime);
    int res = frames[Fk].solver->solve();
    ++nQuery;
    if(res == SAT){
        State *bad_state = new State();
        for(int i=0; i<nInputs; ++i){
            int pipt = (frames[Fk].solver)->val(primed_first_dimacs + i);
            if(pipt > 0)
                bad_state->inputs.push_back(pipt-(primed_first_dimacs-unprimed_first_dimacs));
            else if(pipt < 0)
                bad_state->inputs.push_back(pipt+(primed_first_dimacs-unprimed_first_dimacs));
        }
        for(int i=0; i<nLatches; ++i){
            int l = (frames[Fk].solver)->val(primed_first_dimacs + nInputs + i);
            if(l > 0)
                bad_state->latches.push_back(l-(primed_first_dimacs-unprimed_first_dimacs));
            else if(l < 0)
                bad_state->latches.push_back(l+(primed_first_dimacs-unprimed_first_dimacs));
        }
        extract_state_from_sat(frames[Fk].solver, s, nullptr);
        s->next = bad_state;
        return true;
    }else{
        return false;
    }
}

bool PDR::rec_block_cube(){
    // all the states dealed in rec_block process will not be released.
    vector<State *> states;
    int ct = 0;
    while(!obligation_queue.empty()){
        if(RESULT!=0) return RESULT;
        Obligation obl = *obligation_queue.begin();

        if(output_stats_for_recblock){
            cout << "\nRemaining " << obligation_queue.size() << " Obligation\n";
            cout << "Handling Frames[" << obl.frame_k << "]'s " << "Obligation, depth = " << obl.depth << " , stamp = " << ((obl.state)->index);
            show_state(obl.state);
        }
        // check SAT?[Fk /\ -s /\ T /\ s']
        SATSolver * sat = frames[obl.frame_k].solver;
        if(is_inductive(sat, obl.state->latches, true)){
            if(output_stats_for_recblock){
                cout << "the obligation is already fulfilled, and cube is lifted to ";
                show_litvec(core);
            }
            // latches is inductive to Fk
            obligation_queue.erase(obligation_queue.begin());

            Cube tmp_core = core, tmp_core2 = obl.state->latches;
            generalize(tmp_core, obl.frame_k);
            if(output_stats_for_recblock){
                cout << "the cube is generalized to ";
                show_litvec(tmp_core);
            }

            if (use_punishment and (((obl.state)->next) != nullptr)){
                if(tmp_core.size() > 30){
                    (((obl.state)->next)->failed) = (((obl.state)->next)->failed) + 5;
                }
                else if(tmp_core.size() > 20){
                    (((obl.state)->next)->failed) = (((obl.state)->next)->failed) + 3;
                }
                else if(tmp_core.size() > 10){
                    (((obl.state)->next)->failed) = (((obl.state)->next)->failed) + 1;
                }
            }

            int k;
            for(k = obl.frame_k + 1; k<=depth(); ++k){
                if(!is_inductive(frames[k].solver, tmp_core, false)){
                    break;
                }
            }
            add_cube(tmp_core, k, true, false, k - obl.frame_k + (tmp_core.size() < core.size()));

            if(k <= depth())
               obligation_queue.insert(Obligation(obl.state, k, obl.depth, thread_index));
        }else{
            if(((obl.state)->failed_depth) and ((obl.state)->failed_depth) <= obl.depth + obl.frame_k){
                obligation_queue.erase(obligation_queue.begin());
                if (((obl.state)->next) != nullptr) {
                    (((obl.state)->next)->failed_depth) = ((obl.state)->failed_depth);
                }
                continue;
            }
            if((obl.state)->failed >= 5 and ((obl.depth + obl.frame_k) > depth())){
                obligation_queue.erase(obligation_queue.begin());
                ((obl.state)->failed_depth) = obl.depth + obl.frame_k;
                if (((obl.state)->next) != nullptr) {
                    (((obl.state)->next)->failed_depth) = ((obl.state)->failed_depth);
                }
                continue;
            }

            State *s = new State();
            ++nCTI;
            if(obl.frame_k == 0){
                s->clear();
                for(int i=0; i<nInputs; ++i){
                    int ipt = sat->val(unprimed_first_dimacs + i);
                    if(ipt != 0)
                        s->inputs.push_back(ipt);
                }
                for(int i=0; i<nLatches; ++i){
                    int l = sat->val(unprimed_first_dimacs + nInputs + i);
                    if(l != 0)
                        s->latches.push_back(l);
                }
                s->next = obl.state;
                cex_state_idx = s;
                find_cex = true;
                log_witness();
                return false;
            }else{
                extract_state_from_sat(sat, s, obl.state);
                obligation_queue.insert(Obligation(s, obl.frame_k - 1, obl.depth + 1, thread_index));
            }
        }
    }
    return true;
}

bool PDR::propagate(){
    // all cubes are sorted according to the variable number;
    int start_k = 1;

    if(use_earliest_strengthened_frame){
        if(depth() % 5 and depth() > 20)
            start_k = earliest_strengthened_frame;
    }
    if(top_frame_cannot_reach_bad){
        if(output_stats_for_others) cout << "top frame cannot reach bad" << endl;
        start_k = depth();
    }

    if (output_stats_for_propagate)
        cout << "start to propagate" << endl;

    for(int i=start_k; i<=depth(); ++i){
        int ckeep = 0, cprop = 0;
        for(auto ci = frames[i].cubes.begin(); ci!=frames[i].cubes.end();){
            if(is_inductive(frames[i].solver, *ci, true)){
                ++cprop;
                // should add to frame k+1
                if(core.size() < ci->size())
                    add_cube(core, i+1, true, true, 1);
                else
                    add_cube(core, i+1, false, true, 0);
                auto rm = ci++;
                frames[i].cubes.erase(rm);

            }else{
                ++ckeep;
                ci++;
            }
        }
        if (output_stats_for_propagate)
            cout << "frame="<<i << " ckeep=" << ckeep << " cprop=" << cprop  << endl;
        if(frames[i].cubes.size() == 0){
            if(RESULT != 0) return true;
            RESULT = 20;
            int invariant;
            int new_and_gate;
            bool first_cube = 1;
            Cube badcube;
            badcube.clear();
            badcube.push_back(bad);
            frames[i+1].cubes.insert(badcube);
            for(int l:constraints){
                badcube.clear();
                badcube.push_back(-l);
                frames[i+1].cubes.insert(badcube);
            }
            if(output_stats_for_frames)  show_frames();
            if(print_certifacate){
                for(int d = i+2; d <= depth()+1; d++){
                    for(const Cube &c : frames[d].cubes)
                        frames[i+1].cubes.insert(c);
                }
                for(const Cube &c : frames[i+1].cubes){
                    Cube cc = c;
                    if(cc.size() == 0) cc.push_back(-1);
                    //cc.push_back(-11);
                    //cc.push_back(10);
                    sort(cc.begin(), cc.end(), Lit_CMP());
                    reverse(cc.begin(), cc.end());
                    bool first_bit = 1;
                    //cout << "Cube: ";
                    for(int l:cc){
                        //cout<< l << " ";
                        if(first_bit){
                            new_and_gate = l;
                            first_bit = 0;
                            continue;
                        }
                        int o = 1+nInputs+nLatches+nAnds+1;
                        if(abs(new_and_gate) > abs(l)) ands.push_back(And(o, new_and_gate, l));
                        else ands.push_back(And(o, l, new_and_gate));
                        //cout << "addands " << o << " " << new_and_gate <<" "<< l << endl;
                        new_and_gate = o;
                        nAnds++;
                    }
                    //cout << "new_and_gate = " << new_and_gate << endl << endl;
                    if(first_cube){
                        invariant = -new_and_gate;
                        first_cube = 0;
                        continue;
                    }
                    int o = 1+nInputs+nLatches+nAnds+1;
                    if(abs(new_and_gate) > abs(invariant)) ands.push_back(And(o, -new_and_gate, invariant));
                    else ands.push_back(And(o, invariant, -new_and_gate));
                    //cout << "addands " << o << " " << -new_and_gate <<" "<< invariant << endl;
                    invariant = o;
                    nAnds++;
                }
                bad = -invariant;
                show_aag();
                // if(aiger->binaryMode) show_aig();
                //     else show_aag();
            }
            return true;
        }

    }
    return false;
}

void PDR::mic(Cube &cube, int k, int depth){
    ++nmic;
    int mic_failed = 0;
    set<int> required;
    sort(cube.begin(), cube.end(), Lit_CMP());

    //refer skip
    vector<int> blocker;
    blocker.clear();
    // if(thread_index == 1){  //线程1采用refer_skip技术 refer_skip
    //     for(auto ci = frames[k].cubes.begin(); ci!=frames[k].cubes.end(); ci++){
    //         Cube block_lemma = *ci;
    //         if (includes(cube.begin(), cube.end(), block_lemma.begin(), block_lemma.end())) {
    //             blocker.swap(block_lemma);
    //             break;
    //         }
    //     }
    // }

    //drop literal
    if(thread_index <= 1)  
        stable_sort(cube.begin(), cube.end(), *heuristic_lit_cmp);
    else
        random_shuffle(cube.begin(), cube.end());

    Cube tmp_cube = cube;
    for(int l : tmp_cube){
        vector<int> cand;
        if(find(cube.begin(), cube.end(), l) == cube.end()) {mic_failed = 0; continue;}
        if(refer_skip and find(blocker.begin(), blocker.end(), l) != blocker.end()) continue;
        for(int i : cube){
            if(i != l)
                cand.push_back(i);
        }
        if(output_stats_for_ctg){
             cout << "start ctgdown, level = " << k << ", depth = " << depth << ", cube = ";
             show_litvec(cand);
        }
        if(CTG_down(cand, k, depth, required)){
            if(output_stats_for_ctg){
                cout << "ctgdown successed, the stronger cube is ";
                show_litvec(cand);
            }
            mic_failed = 0;
            cube = cand;
        }else{
            if(output_stats_for_ctg){
                cout << "ctgdown failed, try to remove another lit\n";
            }
            mic_failed++;
            if(mic_failed > option_ctg_tries){
                ++nAbortMic;
                break;
            }
            required.insert(l);
        }
    }
}

bool PDR::CTG_down(Cube &cube, int k, int rec_depth, set<int> &required){
    int ctg_ct = 0;
    int join_ct = 0;
    while(true){
        if(is_init(cube))
            return false;
        // check inductive;
        SATSolver *sat = frames[k].solver;
        if(is_inductive(sat, cube, true)){
            if(output_stats_for_ctg) cout << "The new cube satisfies induction" << endl;
            if(core.size() < cube.size()){
                ++nCoreReduced;
                cube = core;
            }
            return true;
        }
        else{
            if(rec_depth > option_ctg_max_depth)
                return false;
            State *s = new State();
            State *succ = new State(cube, Cube());
            extract_state_from_sat(sat, s, succ);
            int breaked = false;
            if(output_stats_for_ctg){
                cout << "The new cube fails induction, find ctg ";
                show_litvec(s->latches);
            }
            // CTG
            if(ctg_ct < option_ctg_tries && k > 1 && !is_init(s->latches)
                && is_inductive(frames[k-1].solver, s->latches, true)){
                if(output_stats_for_ctg){
                    cout << "ctg satisfies induction, is lifted to ";
                    show_litvec(core);
                }
                Cube &ctg = core;
                ctg_ct ++;
                ++nCTG;
                int i;
                for(i=k; i<=depth(); ++i){
                    if(!is_inductive(frames[i].solver, ctg, false))
                        break;
                }
                int Size =  ctg.size();
                mic(ctg, i-1, rec_depth+1);
                add_cube(ctg, i, true, false, i-k+1 + (ctg.size() < Size));
            }else{
                if(join_ct < option_max_joins){
                    ctg_ct=0;
                    join_ct++;
                    vector<int> join;
                    set<int> s_cti(s->latches.begin(), s->latches.end());
                    for(int i : cube){
                        if(s_cti.find(i) != s_cti.end())
                            join.push_back(i);
                        else if(required.find(i) != required.end()){
                            breaked = true;
                            ++nAbortJoin;
                            break;
                        }
                    }
                    cube = join;
                    if(output_stats_for_ctg){
                        cout << "breaked = "<< breaked << ", ctg cant be removed, join cube and ctg ";
                        show_litvec(cube);
                    }
                }else{
                    breaked = true;
                }
            }
            delete s, succ;
            if(breaked) return false;
        }
    }
}

void PDR::generalize(Cube &cube, int k){
    mic(cube, k, 1);
}

void PDR::clear_po(){
    for(Obligation o : obligation_queue){
        delete o.state;
    }
    obligation_queue.clear();
}

bool PDR::check_BMC0(){
    assert(frames.size() == 0);
    // check SAT?[I /\ Bad] and push F0
    SATSolver *sat0 = new CaDiCaL();
    encode_init_condition(sat0);
    encode_bad_state(sat0);
    sat0->assume(bad);
    int res = sat0->solve();
    if(res == SAT) {
        find_cex = true;
        State *s = new State();
        for(int i=0; i<nInputs; ++i){
            int ipt = sat0->val(unprimed_first_dimacs + i);
            if(ipt != 0) s->inputs.push_back(ipt);
        }
        for(int i=0; i<nLatches; ++i){
            int l = sat0->val(unprimed_first_dimacs + nInputs + i);
            if(l != 0) s->latches.push_back(l);
        }
        cex_states.push_back(s);
        delete sat0;
        return false;
    }
    delete sat0;
    // push F0
    new_frame();
    encode_init_condition(frames[0].solver);
    return true;
}

bool PDR::check_BMC1(){
    assert(frames.size() == 1);
    // push Foo
    // check SAT?[I /\ T /\ Bad'] and push Foo
    SATSolver *sat1 = new CaDiCaL();
    encode_init_condition(sat1);
    encode_translation(sat1);
    for(int l:constraints_prime){
        sat1->add(l);
        sat1->add(0);
    }
    sat1->assume(bad_prime);
    int res1 = sat1->solve();
    if(res1 == SAT){
        find_cex = true;
        State *s = new State();
        for(int i=0; i<nInputs; ++i){
            int ipt = sat1->val(unprimed_first_dimacs + i);
            if(ipt != 0) s->inputs.push_back(ipt);
        }
        for(int i=0; i<nLatches; ++i){
            int l = sat1->val(unprimed_first_dimacs + nInputs + i);
            if(l != 0) s->latches.push_back(l);
        }

        State *ps = new State();
        for(int i=0; i<nInputs; ++i){
            int pipt = sat1->val(primed_first_dimacs + i);
            if(pipt > 0)
                ps->inputs.push_back(pipt-(primed_first_dimacs-unprimed_first_dimacs));
            else if(pipt < 0)
                ps->inputs.push_back(pipt+(primed_first_dimacs-unprimed_first_dimacs));
        }
        for(int i=0; i<nLatches; ++i){
            int l = sat1->val(primed_first_dimacs + nInputs + i);
            if(l > 0)
                ps->latches.push_back(l-(primed_first_dimacs-unprimed_first_dimacs));
            else if(l < 0)
                ps->latches.push_back(l+(primed_first_dimacs-unprimed_first_dimacs));
        }

        s->next = ps;
        cex_states.push_back(s);
        cex_states.push_back(ps);
        delete sat1;
        return false;
    }       
    delete sat1;
    new_frame();
    return true;
}

int PDR::check(){
    initialize();

    if(!check_BMC0()) {
        std::lock_guard<std::mutex> lock(result_mutex);
        if(!no_output) cout << "Thread " << thread_index << " check BMC0 failed" << endl;
        if(RESULT == 0){
            RESULT = 10;
            show_witness();
        }
        return 10;
    }
    if(!no_output) cout << "Thread " << thread_index << " check BMC0 successed" << endl;
    if(!check_BMC1()) {
        std::lock_guard<std::mutex> lock(result_mutex);
        if(!no_output) cout << "Thread " << thread_index << " check BMC1 failed" << endl;
        if(RESULT == 0){
            RESULT = 10;
            show_witness();
        }
        return 10;
    }
    if(!no_output) cout << "Thread " << thread_index << " check BMC1 successed" << endl;
    // main loop of IC3, start from depth = 1.
    // Fk need to hold -Bad all the time
    new_frame();
    assert(depth() == 1);
    top_frame_cannot_reach_bad = true;
    earliest_strengthened_frame = depth();
    int result = 10;
    int ct = 0;
    while(true){
        if(RESULT!=0) return RESULT;
        if(share_memory and main_thread_index >= 0){
            while(!cube_register[main_thread_index][depth()].empty()){
                if(RESULT!=0) return RESULT;
                share_Cube c = cube_register[main_thread_index][depth()].front();
                cube_register[main_thread_index][depth()].pop();
                Cube dequeue_cube = c.share_cube;
                pair<set<Cube, Cube_CMP>::iterator, bool> rv = frames[depth()].cubes.insert(dequeue_cube);
                if(!rv.second) continue;
                if(c.toall){
                    for(int index=1; index<depth(); index++){
                        for(int l : dequeue_cube)
                            frames[index].solver->add(-l);
                        frames[index].solver->add(0);
                    }
                }
                for(int l : c.share_cube)
                    frames[depth()].solver->add(-l);
                frames[depth()].solver->add(0);
            }
            for(int thread=0; thread<=3; thread++){
                while(!cube_producer[main_thread_index][thread].empty()){
                    if(RESULT!=0) return RESULT;
                    share_Cube c;
                    cube_producer[main_thread_index][thread].pop(c);
                    if(c.frame > depth()) {cube_register[main_thread_index][c.frame].push(c); continue;}
                    Cube dequeue_cube = c.share_cube;
                    pair<set<Cube, Cube_CMP>::iterator, bool> rv = frames[c.frame].cubes.insert(dequeue_cube);
                    if(!rv.second) continue;
                    cout << "thread" + to_string(thread) + " dequeue into " + to_string(1) + " " + return_litvec(dequeue_cube) +"\n";
                    if(c.toall){
                        for(int index=1; index<c.frame; index++){
                            for(int l : dequeue_cube)
                                frames[index].solver->add(-l);
                            frames[index].solver->add(0);
                        }
                    }
                    for(int l : c.share_cube)
                        frames[c.frame].solver->add(-l);
                    frames[c.frame].solver->add(0);
                }
            }
        }

        // 查询坏状态 get states which are one step to bad
        State *s = new State();
        bool flag = get_pre_of_bad(s);
        if(flag){
            ++nCTI;
            obligation_queue.clear();
            obligation_queue.insert(Obligation(s, depth()-1, 1, thread_index));
            top_frame_cannot_reach_bad = false;
            if(!rec_block_cube()){
                // find counter-example
                result = 10;
                if(RESULT==0) {
                    RESULT = 10;
                    show_witness();
                }
                break;
            }else{
                for(State *p : states) delete p;
            }
        }
        else{
            assert(obligation_queue.size() == 0);
            if(output_stats_for_recblock) cout << "CTI not found\n";
            if(propagate()){
                // find invariants
                result = 20;
                RESULT = 20;
                break;
            }
            if(output_stats_for_conclusion){
                cout << ". # Level        " << depth() << endl;
                cout << ". # Queries:     " << nQuery << endl;
                cout << ". # CTIs:        " << nCTI << endl;
                cout << ". # CTGs:        " << nCTG << endl;
                cout << ". # mic calls:   " << nmic << endl;
                cout << ". # Red. cores:  " << nCoreReduced << endl;
                cout << ". # Abort joins: " << nAbortJoin << endl;
                cout << ". # Abort mics:  " << nAbortMic << endl;
            }
            new_frame();
            top_frame_cannot_reach_bad = true;
            earliest_strengthened_frame = depth();
            if(!no_output) cout << "pdr" + to_string(thread_index) + "_step = " +  to_string(earliest_strengthened_frame) + "\n";
        }
    }
    int d = depth();
    if(!no_output) cout <<  "depth" + to_string(thread_index) + " = " + to_string(d) + "\n";

    for(auto f : frames){
        if(f.solver != nullptr)
            delete f.solver;
    }
    frames.clear();

    return result;
}
