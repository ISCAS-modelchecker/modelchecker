#pragma once
#include "./ipasir.h"
#include "./minisat/simp/SimpSolver.h"
#include <vector>
#include <sstream>
#include <fstream>
using std::vector;

#define SAT 10
#define UNSAT 20
#define UNKNOWN 0
class SATSolver{
public:
    virtual ~SATSolver() = default;
    virtual void add(int dimacs_lit)=0;
    virtual void assume(int assumption_lit)=0;
    virtual int solve()=0;
    virtual int failed(int lit)=0;
    virtual int val(int lit)=0;
    virtual int max_var()=0;
    void set_clear_act(){};
    void clear_act(){};
    virtual void show_info()=0;
};

class CaDiCaL: public SATSolver{
    void * s;
    vector<int > cls;
    int nv;
    bool clear_flag;
public:
    CaDiCaL(){
        s = ipasir_init();
        nv = 0;
        clear_flag = false;
    }
    ~CaDiCaL(){
        ipasir_release(s);
    }
    void add(int dimacs_lit){
        ipasir_add(s, dimacs_lit);
        nv = max(nv, abs(dimacs_lit));
        // if(dimacs_lit == 0){
        //     cout<<"add_cls: ";
        //     for(int l : cls){
        //         cout<<l<<" ";
        //     }cout<<endl;
        //     cls.clear();
        // }else{
        //     cls.push_back(dimacs_lit);
        // }
    }
    void assume(int assumption_lit){
        ipasir_assume(s, assumption_lit);
    }
    int solve(){
        return ipasir_solve(s);
    }
    int failed(int lit){
        return ipasir_failed(s, lit);
    }
    int val(int lit){
        return ipasir_val(s, lit);
    }
    int max_var(){
        return nv;
    }
    void set_clear_act(){
        clear_flag = true;
    }
    void clear_act(){
        if(clear_flag){
            this->add(-nv);
            this->add(0);
        }
        clear_flag = false;
    }
    void show_info(){};
};


class minisatSimp: public SATSolver{
    void *s;
    Minisat::vec<Minisat::Lit> clause;
    Minisat::vec<Minisat::Lit> assumptions;
    vector<vector<int>> witness;
public:
    vector<int> simplified_cnf;
    
    minisatSimp(){
        s = new Minisat::SimpSolver();
        ((Minisat::SimpSolver*)s)->random_seed = 0;
    }
    
    void var_enlarge_to(int v){
        while(((Minisat::SimpSolver *)s)->nVars() < v )
            ((Minisat::SimpSolver *)s)->newVar();
    }

    void add(int dimacs_lit){
        if(dimacs_lit == 0){
            // printf("add_cls: ");
            // for(int i = 0; i<clause.size(); ++i){
            //     int v = Minisat::var(clause[i]) + 1;
            //     int l = Minisat::sign(clause[i]) ? -v : v;
            //     printf("%d ", l);
            // }puts("");

            ((Minisat::SimpSolver *)s)->addClause(clause);
            clause.clear();

        }else{
            Minisat::Var var_idx = abs(dimacs_lit);
            var_enlarge_to(var_idx);
            Minisat::Lit l = Minisat::mkLit(var_idx-1, dimacs_lit<0);
            clause.push(l);
        }
    }
    void assume(int assumption_lit){
        Minisat::Var var_idx = abs(assumption_lit) - 1;
        Minisat::Lit l = Minisat::mkLit(var_idx, assumption_lit<0);
        assumptions.clear();
    }
    int solve(){
        bool res = ((Minisat::SimpSolver *)s)->solve(assumptions);
        assumptions.clear();
        if(res)
            return 10;
        else
            return 20;
    }
    int failed(int lit){
        Minisat::Var var_idx = abs(lit) - 1;
        Minisat::Lit l = Minisat::mkLit(var_idx, lit>0);
        if( ((Minisat::SimpSolver *)s)->conflict.has(l)){
            return 1;
        }else{
            return 0;
        }
    }
    int val(int lit){
        Minisat::Var var_idx = abs(lit) - 1;
        Minisat::lbool lb = ((Minisat::SimpSolver *)s)->modelValue(var_idx);
        if(lb == Minisat::l_True){
            return abs(lit);
        }else if(lb == Minisat::l_False){
            return -abs(lit);
        }else{
            return 0;
        }
    }

    void set_frozen(int lit){
        int v = abs(lit);
        var_enlarge_to(v);
        // cout<<"F"<<v<<endl;
        ((Minisat::SimpSolver *)s)->setFrozen(v-1, true);
    }

    void simplify(){

        // cout<<((Minisat::SimpSolver *)s)->nVars()<<endl;
        // cout<<((Minisat::SimpSolver *)s)->nClauses()<<endl;

        ((Minisat::SimpSolver *)s)->eliminate();
        
        // int max_var = 0;
        // cout<<((Minisat::SimpSolver *)s)->nVars()<<endl;
        // cout<<((Minisat::SimpSolver *)s)->nClauses()<<endl;
        
        simplified_cnf.clear();
        simplified_cnf.push_back(((Minisat::SimpSolver *)s)->nVars());
        simplified_cnf.push_back(-((Minisat::SimpSolver *)s)->nVars());
        simplified_cnf.push_back(0);
        for (Minisat::ClauseIterator c = ((Minisat::SimpSolver *)s)->clausesBegin(); 
            c != ((Minisat::SimpSolver *)s)->clausesEnd(); ++c) {
                const Minisat::Clause & cls = *c;
                for (int i = 0; i < cls.size(); ++i){
                    int v = Minisat::var(cls[i]) + 1;
                    // max_var = max(v, max_var);
                    int l = (Minisat::sign(cls[i])? -v : v);
                    // cout<<l<<" ";
                    simplified_cnf.push_back(l);
                }
                // cout<<endl;
                simplified_cnf.push_back(0);
            }
        for (Minisat::TrailIterator c = ((Minisat::SimpSolver *)s)->trailBegin(); 
            c != ((Minisat::SimpSolver *)s)->trailEnd(); ++c){
                int v = Minisat::var(*c) + 1;
                // max_var = max(v, max_var);
                int l = (Minisat::sign(*c)? -v : v);
                simplified_cnf.push_back(l);
                simplified_cnf.push_back(0);
                // cout<<l<<endl;
        }
        // exit(0);        
        // cout<<"simp max :";
        // cout<<max_var<<endl;
    }
    
    int max_var(){
        return ((Minisat::SimpSolver *)s)->nVars();
    }
    void show_info(){};

};




class minisatCore: public SATSolver{
    void *s;
    Minisat::vec<Minisat::Lit> clause;
    Minisat::vec<Minisat::Lit> assumptions;
    vector<vector<int>> witness;
    int nv;
    bool clear_flag = false;
public:
    vector<int> simplified_cnf;
    minisatCore(){
        s = new Minisat::Solver();
        nv = 0;
        clear_flag = false;
    }
    void show_info(){
        cout<<((Minisat::Solver *)s)->nVars()<<" ";
        cout<<((Minisat::Solver *)s)->nClauses()<<endl;
    }

     void var_enlarge_to(int v){
        // if(((Minisat::Solver *)s)->nVars() < v)
        //     cout<<((Minisat::Solver *)s)->nVars()<<" to "<<v<<endl;
        while(((Minisat::Solver *)s)->nVars() < v )
            ((Minisat::Solver *)s)->newVar();
    }
    void add(int dimacs_lit){
        if(dimacs_lit == 0){
            // printf("add_cls: ");
            // for(int i = 0; i<clause.size(); ++i){
            //     int v = Minisat::var(clause[i]) + 1;
            //     int l = Minisat::sign(clause[i]) ? -v : v;
            //     printf("%d ", l);
            // }puts("");

            ((Minisat::Solver *)s)->addClause(clause);
            clause.clear();

        }else{
            nv = max(nv, abs(dimacs_lit));
            Minisat::Var var_idx = abs(dimacs_lit) - 1;
            var_enlarge_to(nv);
            Minisat::Lit l = Minisat::mkLit(var_idx, dimacs_lit<0);
            clause.push(l);
        }
    }
    void assume(int assumption_lit){
        Minisat::Var var_idx = abs(assumption_lit) - 1;
        nv = max(nv, abs(assumption_lit));
        var_enlarge_to(nv);
        Minisat::Lit l = Minisat::mkLit(var_idx, assumption_lit<0);
        assumptions.push(l);
    }
    int solve(){
        bool res = ((Minisat::Solver *)s)->solve(assumptions);
        assumptions.clear();
        if(res)
            return 10;
        else
            return 20;
    }
    int failed(int lit){
        Minisat::Var var_idx = abs(lit) - 1;
        Minisat::Lit l = Minisat::mkLit(var_idx, lit>0);

        if( ((Minisat::Solver *)s)->conflict.has(l)){
            return 1;
        }else{
            return 0;
        }
    }
    int val(int lit){
        Minisat::Var var_idx = abs(lit) - 1;
        Minisat::lbool lb = ((Minisat::Solver *)s)->modelValue(var_idx);
        if(lb == Minisat::l_True){
            return abs(lit);
        }else if(lb == Minisat::l_False){
            return -abs(lit);
        }else{
            return 0;
        }
    }

    int max_var(){
        // cout << "max_var = "<<((Minisat::Solver *)s)->nVars()<<endl;
        assert(((Minisat::Solver *)s)->nVars() == nv);
        return nv;
    }

    void set_clear_act(){
        clear_flag = true;
    }
    void clear_act(){
        // cout<<"clear act flag "<<clear_flag<<endl;
        if(clear_flag){
            this->add(-nv);
            this->add(0);
        }
        clear_flag = false;
    }
    void show_clauses(){
        cout<<"show_lift clauses begin"<<endl;
    
        for (Minisat::ClauseIterator c = ((Minisat::SimpSolver *)s)->clausesBegin(); 
            c != ((Minisat::SimpSolver *)s)->clausesEnd(); ++c) {
                const Minisat::Clause & cls = *c;
                for (int i = 0; i < cls.size(); ++i){
                    int v = Minisat::var(cls[i]) + 1;
                    int l = (Minisat::sign(cls[i])? -v : v);
                    cout<<l<<" ";
                }
                cout<<endl;
            }
        for (Minisat::TrailIterator c = ((Minisat::SimpSolver *)s)->trailBegin(); 
            c != ((Minisat::SimpSolver *)s)->trailEnd(); ++c){
                int v = Minisat::var(*c) + 1;
                int l = (Minisat::sign(*c)? -v : v);
                cout<<l<<endl;
        }
        
        cout<<"show_lift clauses end"<<endl;
    }
};

