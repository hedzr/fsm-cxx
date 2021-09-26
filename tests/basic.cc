//
// Created by Hedzr Yeh on 2021/9/26.
//

#include "fsm_cxx.hh"

#include <iostream>
#include <math.h>
#include <string>

#include <functional>
#include <memory>
#include <random>

namespace hicc { namespace dp { namespace state { namespace basic {
}}}} // namespace hicc::dp::state::basic
namespace hicc { namespace dp { namespace state { namespace bugs {
    int v = 0;
}}}} // namespace hicc::dp::state::bugs

void test_state_basic() {
    using namespace hicc::dp::state::basic;
}

namespace fsm_cxx { namespace test {
    AWESOME_MAKE_ENUM(my_state,
                      Empty,
                      Error,
                      Initial,
                      Terminated,
                      Opened,
                      Closed)

    struct begin {};
    struct end {};
    struct open {};
    struct close {};

    void test_state_meta() {
        // using namespace hicc::dp::state::meta;
        // using namespace hmeta;
        machine_t<my_state> m;

        m.initial(my_state::Initial)
                .terminated(my_state::Terminated)
                .error(my_state::Error);
        m.transition(my_state::Initial, begin{}, my_state::Closed)
                .transition(my_state::Closed, open{}, my_state::Opened)
                .transition(my_state::Opened, close{}, my_state::Closed)
                .transition(my_state::Closed, end{}, my_state::Terminated);
        m.transition(my_state::Opened,
                     decltype(m)::Transition{end{}, my_state::Terminated, nullptr,
                                             [](context_t<my_state> &, my_state) { std::cout << "<T><END>"; }});
        m.on_action_for_debug([m](auto const &from, auto const &ev, auto const &to, auto const &actions) {
            std::printf("        [%s] -- %s --> [%s]\n", m.state_to_sting(from).c_str(), ev.c_str(), m.state_to_sting(to).c_str());
            UNUSED(actions);
        });

        // processing

        m.step_by(begin{});
        m.step_by(open{});
        m.step_by(close{});
        m.step_by(end{});
    }
}} // namespace fsm_cxx::test

namespace lambdas {
    void f(int n1, int n2, int n3, const int &n4, int n5) {
        std::cout << n1 << ' ' << n2 << ' ' << n3 << ' ' << n4 << ' ' << n5 << '\n';
    }

    int g(int n1) { return n1; }

    struct Foo {
        void print_sum(int n1, int n2) {
            std::cout << n1 + n2 << '\n';
        }
        int data = 10;
    };

    void test_lambdas() {
        {
            using namespace std::placeholders;
            std::vector<std::function<int(int)>> functions;

            std::function<int(int, int)> foo = [](int a, int b) { return a + b; };
            std::function<int(int)> bar = std::bind(foo, 2, std::placeholders::_1);
            std::function<int(int, int)> bar2args = std::bind(foo, _1, _2);

            functions.push_back(bar);
            UNUSED(bar2args);
        }

        {
            using namespace std::placeholders;
            using namespace lambdas;

            // 演示参数重排序和按引用传递
            int n = 7;
            // （ _1 与 _2 来自 std::placeholders ，并表示将来会传递给 f1 的参数）
            auto f1 = std::bind(f, _2, 42, _1, std::cref(n), n);
            n = 10;
            f1(1, 2, 1001); // 1 为 _1 所绑定， 2 为 _2 所绑定，不使用 1001
            // 进行到 f(2, 42, 1, n, 7) 的调用

            // 嵌套 bind 子表达式共享占位符
            auto f2 = std::bind(f, _3, std::bind(g, _3), _3, 4, 5);
            f2(10, 11, 12); // 进行到 f(12, g(12), 12, 4, 5); 的调用

            // 常见使用情况：以分布绑定 RNG
            std::default_random_engine e;
            std::uniform_int_distribution<> d(0, 10);
            std::function<int()> rnd = std::bind(d, e); // e 的一个副本存储于 rnd
            for (int n1 = 0; n1 < 10; ++n1)
                std::cout << rnd() << ' ';
            std::cout << '\n';

            // 绑定指向成员函数指针
            Foo foo;
            auto f3 = std::bind(&Foo::print_sum, &foo, 95, _1);
            f3(5);

            // 绑定指向数据成员指针
            auto f4 = std::bind(&Foo::data, _1);
            std::cout << f4(foo) << '\n';

            // 智能指针亦能用于调用被引用对象的成员
            std::cout << f4(std::make_shared<Foo>(foo)) << '\n'
                      << f4(std::make_unique<Foo>(foo)) << '\n';
        }
    }
} // namespace lambdas


int main() {

    // lambdas::test_lambdas();
    test_state_basic();
    fsm_cxx::test::test_state_meta();

    return 0;
}