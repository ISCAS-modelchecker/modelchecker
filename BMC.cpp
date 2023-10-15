#include "BMC.hpp"
#include "sat_solver.hpp"
#include <assert.h>
#include <sstream>
#include <algorithm>
#define value(rc) (rc > 0 ? values[rc] : -values[-rc])  //原始变量 -> 展开变量

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
    cout << "nAnds = " << nAnds << ", ands.size() = " << ands.size() << endl;
    for(int i=0; i<ands.size(); ++i){
        cout << "ands[" << i << "] = " << ands[i].o << " " << ands[i].i1 << " " << ands[i].i2 <<endl;
        if(i>1000) break;
    }
}

void BMC::show_nexts(){
    cout << "-------------show_nexts_inits------------------" <<endl;
    cout << "nlatches = " << nLatches << ", nexts.size() = " << nexts.size() << ", inits.size() = " << init_state.size() << endl;
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
        //cout << l << " "; l = -6 -7 -8 -9 -10 即aiger的五个latch初始0
        bmcSolver->add(l); bmcSolver->add(0);
    }

    if(constraints.size() >= 0){
        for(int l : constraints){ //-5 -4 -3 即3个input为0
            s->add(l); s->add(0);}

        set<int> lit_set;
        for(int l : constraints)
            lit_set.insert(abs(l));

        for(auto i = ands.rbegin(); i != ands.rend(); ++i){
            And & a = *i;
            if(lit_set.find(a.o) == lit_set.end())
                continue;
            lit_set.insert(abs(a.i1));
            lit_set.insert(abs(a.i2));//??????
            
            s->add(-a.o); s->add(a.i1);  s->add(0);
            s->add(-a.o); s->add(a.i2);  s->add(0);
            s->add(a.o);  s->add(-a.i1); s->add(-a.i2); s->add(0);
        }
    }
    int res = bmcSolver->solve();
    cout << "check init result = " << res << endl;
}

// translate the aiger language to internal states
void BMC::translate_to_dimacs(){
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
    for(int i=0; i<(aiger->constraints).size(); ++i){
        int cst = aiger->constraints[i];
        constraints.push_back(aiger_to_dimacs(cst));
    }
    //show_constraints();

    // load bad states
    if(aiger->num_bads > 0 && aiger->num_bads > property_index){
        int b = aiger->bads[property_index];
        bad = aiger_to_dimacs(b);
        for(int i=0; i<(aiger->bads).size(); i++){
            b = aiger->bads[i];
            allbad.push_back(aiger_to_dimacs(b));
        }
    }else if(aiger->num_outputs > 0 && aiger->num_outputs > property_index){
        int output = aiger->outputs[property_index];
        bad = aiger_to_dimacs(output);
        for(int i=0; i<(aiger->outputs).size(); i++){
            output = aiger->outputs[i];
            allbad.push_back(aiger_to_dimacs(output));
        }
    }else{
        assert(false);
    }
    assert(abs(bad) <= variables.size());

    //show_bads();

    //show_constraints();

    //show_variables();
    
    //show_ands();

    //show_nexts();
}

// lc 和 rc 是一个与门的两个子节点 可正可负 可为and可为latch/input
int BMC::Aig_And(int p0, int p1){  // 输入原始变量p0, p1, return 展开变量p0 * p1
    int first_ands_index = nInputs + nLatches + 2;     // = ands[0].o
    int v0 = value(p0), v1 = value(p1);
    
    //check trivial cases 特殊情况 
    if ( v0 == v1 )
        { return v0;}
    if ( v0 == -v1 )
        { return 1;}
    if ( abs(v0) == 1 )
        { return v0 == -1 ? v1 : 1;}
    if ( abs(v1) == 1 )
        { return v1 == -1 ? v0 : 1;} 

    // check not so trivial cases: p0 或 p1 都是and gate
    int pfana, pfanb, pfanc, pfand, va, vb, vc, vd;    //子节点的子节点 同样可正可负 可为and可为latch/input, pdana是原始变量 va是对应的展开变量
    if (abs(p0) >= first_ands_index){ //p0 是与门
        pfana = ands[abs(p0) - first_ands_index].i1; va = value(pfana);
        pfanb = ands[abs(p0) - first_ands_index].i2; vb = value(pfanb);
    }
    else{                             //p0 是latch/input
        pfana = abs(p0); va = value(pfana);
        pfanb = -1;      vb = value(pfanb);
    }
    if (abs(p1) >= first_ands_index){ //p1 是与门
        pfanc = ands[abs(p1) - first_ands_index].i1; vc = value(pfanc);
        pfand = ands[abs(p1) - first_ands_index].i2; vd = value(pfand);
    }
    else{                             //p0 是latch/input
        pfanc = abs(p1); vc = value(pfanc);
        pfand = -1;      vd = value(pfand);
    }
    //cout << "childs:" << pfana << "  " << pfanb << "  " << pfanc << "  " << pfand << "  " << va << "  " << vb << "  " << vc << "  " << vd << "  " <<endl;
    //cout << pfand << "  " << p0 << "  " << vd << "  " << v0  <<endl;
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
    //deal with inputs(unfold_variables加入新一轮input)
    for(int i=0; i<nInputs; ++i){
        uaiger->nodes.push_back(Node(2, 0, 0, 0));      //uaiger->unfold_variables.push_back(Variable((uaiger->vsize()), 'i', (uaiger->isize()), false));
        uaiger->inputs.push_back(uaiger->nsize()-1);
        values[i+2] = uaiger->nsize()-1;
    }

    //deal with ands o = ia * ib
    if(unfold_ands) cout << "unfold ands" << endl;
    for(int i=0; i<nAnds; ++i){
        assert(ands[i].o == i+nInputs+nLatches+2);
        values[ands[i].o] = 0;

        //优先考虑化简与门 
        values[ands[i].o] = Aig_And(ands[i].i1, ands[i].i2);
        if (unfold_ands and values[ands[i].o] != 0 and i<500){
            cout << ands[i].o << " " << ands[i].i1 << " " << ands[i].i2 << "->";
            cout << "values[" << ands[i].o << "] = " << values[ands[i].o] << endl;
            continue;
        }
        
        //化简与门失败
        if (values[ands[i].o] == 0){
            //获取原电路的与门o = ia * ib的当前值 存入新与门
            int i1, i2, output = 0;
            i1 = value(ands[i].i1);
            i2 = value(ands[i].i2);
            // 查找i1的父节点是否存在相同的子节点i2（等价性验证）
            for(int k=0; k<(uaiger->hash_table[abs(i1)]).size(); k++ ){
                //if(i > 500) break;
                if( (uaiger->hash_table[abs(i1)][k]).i1 == i1 && (uaiger->hash_table[abs(i1)][k]).i2 == i2){
                    //cout << "a " << i1  << " " << i2 << endl;
                    values[i+nInputs+nLatches+2] = (uaiger->hash_table[abs(i1)][k]).o;
                    output = (uaiger->hash_table[abs(i1)][k]).o;
                }
                else if( (uaiger->hash_table[abs(i1)][k]).i2 == i1 && (uaiger->hash_table[abs(i1)][k]).i1 == i2){
                    //cout << "b " << i1  << " " << i2 << endl;
                    values[i+nInputs+nLatches+2] = (uaiger->hash_table[abs(i1)][k]).o;
                    output = (uaiger->hash_table[abs(i1)][k]).o;
                }  
            }
            //不存在已有的等价节点
            if (values[ands[i].o] == 0){
                //化简与门失败 新建and gate 变量x 变量索引等于变量id
                uaiger->nodes.push_back(Node(3, 0, i1, i2));        //uaiger->unfold_variables.push_back(Variable((uaiger->vsize()), 'a', (uaiger->asize()), false));
                uaiger->nodes[abs(i1)].fathers++;
                uaiger->nodes[abs(i2)].fathers++;

                //记录与门o的当前值为x 
                values[i+nInputs+nLatches+2] = uaiger->nsize()-1;
                output = uaiger->nsize()-1;

                (uaiger->ands).push_back(And(output, i1, i2));
                (uaiger->hash_table[abs(i1)]).push_back(And(output, i1, i2));
                (uaiger->hash_table[abs(i2)]).push_back(And(output, i1, i2));
            }
            //如果与门是不变式约束节点 则fathers+1
            //if(and_is_cons == 1)  uaiger->nodes[abs(values[ands[i].o])].fathers++;
            //log
            if(unfold_ands && i<500)    cout << ands[i].o << " " << ands[i].i1 << " " << ands[i].i2 << "->" << output << " " << i1 << " " << i2 << endl;   
        }
    }
    //deal with constraints
    for(int i=0; i<constraints.size(); ++i){
        (uaiger->constraints).push_back(value(constraints[i]));
    }
    //deal with output
    for(int index=0; index<allbad.size(); index++){
        int newbad = value(allbad[index]);
        uaiger->outputs.push_back(newbad); //uaiger->outputs.push_back(value(bad));
        uaiger->nodes[abs(newbad)].fathers++; //uaiger->nodes[abs(value(bad))].fathers++;
    }
    //deal with register
    if(unfold_latches) cout << "unfold latches" << endl;
    for(int i=0; i<=nLatches-1; ++i){
        tempvalue[i] = value(nexts[i]);   
    }
    for(int i=0; i<=nLatches-1; ++i){
        values[i+nInputs+2] = tempvalue[i];
        if(unfold_latches) cout << nexts[i] << "-> " << values[i+nInputs+2] << endl;  
    }  
}

void BMC::initialize(){
    cout<<"c BMC constructed from aiger file [Finished] "<<endl; 
    translate_to_dimacs();
    cout << "start BMC initialize" <<endl;

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
    bmcSolver->add(-1); bmcSolver->add(0); 
    lit_has_insert.resize(99999999);       //lit_has_insert.resize((uaiger->ands).size()+2); 
    bmc_frame_k = 0;

    //unfold init
    cout << "start BMC unfold" <<endl;
    // 加入 NULL 和 常量false  
    uaiger->nodes.push_back(Node(0, 0, 0, 0));  // uaiger->unfold_variables.push_back(Variable(0, string("NULL")));
    uaiger->nodes.push_back(Node(1, 0, 0, 0));  // uaiger->unfold_variables.push_back(Variable(1, string("False")));
    //deal with init latches 存储latch的真实初始值(init = false = x1 或 init = true = -x1)
    values.resize(variables.size());
    values[1] = 1;                    // x1 = false                  
    for(int latch: init_state){
        if(latch > 0) values[latch] = -1;
            else if(latch < 0) values[-latch] = 1;
    }
    for(int i=0; i<=nLatches-1; ++i){
        if(values[i+nInputs+2] == 0){
            uaiger->nodes.push_back(Node(2, 0, 0, 0)); //uaiger->unfold_variables.push_back(Variable((uaiger->vsize()), 'l', 0, false));
            values[i+nInputs+2] = uaiger->nsize()-1;            
        } 
    }
}

//check all frames
int BMC::check(){
    int res;
    for(bmc_frame_k = 1; bmc_frame_k <= nframes; bmc_frame_k++){
        unfold();
        res = solve_one_frame();
        if (res == 10) {
            uaiger->show_statistics();
            cout << "Output was asserted in frame." << endl;
            return 1;
        }   
    } 
    // res = 0
    uaiger->show_statistics();
    cout << "No output asserted in frames." << endl; 
    return 0;
}

// check one frame
int BMC::solve_one_frame(){
    int bad = (uaiger->outputs).back();
    cout << "frames = "<< bmc_frame_k <<", bad = " << bad << ", res = ";

    set<int> lit_set;
    lit_set.insert(abs(bad));
    //for(int cst : constraints) lit_set.insert(abs(cst));
    
    for(int i = (uaiger->ands).size()-1; i>=0; i--){   
        And a = uaiger->ands[i];            
        //判断这个与门是否被add过 判断这个与门是否需要是COI
        if(lit_has_insert[i] == 1 || lit_set.find(a.o) == lit_set.end()){
            continue;
        }

        lit_set.erase(a.o);        
        lit_has_insert[i] = 1;
        lit_set.insert(abs(a.i1));
        lit_set.insert(abs(a.i2));
        
        bmcSolver->add(-a.o); bmcSolver->add(a.i1);  bmcSolver->add(0);
        bmcSolver->add(-a.o); bmcSolver->add(a.i2);  bmcSolver->add(0);
        bmcSolver->add(a.o);  bmcSolver->add(-a.i1); bmcSolver->add(-a.i2); bmcSolver->add(0);
    }

    bmcSolver->assume(bad);
    int result = bmcSolver->solve();
    if(result == 20){
        cout << result << endl;
        bmcSolver->add(-bad); bmcSolver->add(0); 
    } 
    else if(result == 10){
        cout << result << endl;      
    }
    else cout << "???" << result << endl;
    return result;
}