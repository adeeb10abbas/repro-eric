// Goal: See if it's possible to have a class compose itself, and implicitly accept initializer lists
// Purpose: See if its possible to simplify DecisionVariable stuff
//  Explicitly constrian DecisionVar, such that that is not part of the template generalization

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "name_trait.h"

using std::string;
using std::cout;
using std::endl;
using std::vector;

class Var {
public:
    inline Var(const string& name)
        : name_(name)
    { }
protected:
    string name_;
};

class Vars {
public:
    inline Vars(const string& s)
        : vars_{{s}}
    { }
    inline Vars(const Var& var)
        : vars_{var}
    { }
    template<typename T>
    inline Vars(const vector<T>& items)
    {
        for (const auto& item : items)
            add_var(item);
    }
    // Explicit intiailize_list
    inline Vars(std::initializer_list<string> list)
        : Vars(vector<string>(list))
    { }
protected:
    vector<Var> vars_;
    // void add_var(const Var& v) {
    //     vars_.push_back(v);
    // }
    void add_var(const Vars& vs) {
        for (const auto& v : vs.vars_)
            vars_.push_back(v);
    }
};

int main() {
    Var var {"a"};
    Vars vars = {"a", "b"}; // {"a"} does not work
    // Vars vars_nest = vector<Vars>({vars, vars, "a"});
    // Vars vars2("a"); // does not work
    return 0;
}
