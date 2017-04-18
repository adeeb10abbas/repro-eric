#include <iostream>

// Try to replicate: http://en.cppreference.com/w/cpp/language/class_template#Explicit_instantiation
// Can do so with:   http://en.cppreference.com/w/cpp/language/function_template#Explicit_instantiation

template<typename T>
struct tpl_traits {
    static const char* name() {
        return "generic trait";
    }
};

// Providing definition here prevents other implementations of tpl_traits from being used
// Example: move this definiton here, and observe that the traits are still generic
// Does this mean it isn't using the specializations from `tpl_inst.cc`? How do I inspect this?
template<typename T>
void tpl_func(const T& x);

// Specialized methods
struct test {
    // Explicitly rely on external definition (in source file)
    template<typename T>
    void tpl_method_source(const T& x);

    // Use header definition, but explicitly instantiate in source file
    template<typename T>
    void tpl_method_source_spec(const T& x) {
        std::cout << "header default impl <" << tpl_traits<T>::name() << ">" << std::endl;
    }
};

template<typename ... Args>
void tpl_func_var(Args ... args);

// Explicitly instantiate in source file
extern template
void test::tpl_method_source_spec<int>(const int& x);

// Also use a header specialization
template<>
void test::tpl_method_source_spec<double>(const double& x) {
    std::cout << "header double impl" << std::endl;
}
