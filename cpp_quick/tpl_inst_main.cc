#include "tpl_inst.h"

int main() {
    int x {};
    double y {};

    tpl_func(x);
    tpl_func(y);

    test t;
    t.tpl_method_source(x);
    // t.tpl_method_source(y);

    t.tpl_method_source_spec(x);
    t.tpl_method_source_spec(y);

    tpl_func_var(x, y);
    return 0;
}
