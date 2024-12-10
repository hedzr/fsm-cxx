// fsm_cxx Library
// Copyright © 2021 Hedzr Yeh.
//
// This file is released under the terms of the MIT license.
// Read /LICENSE for more information.

//
// Created by Hedzr Yeh on 2021/9/26.
//

#include "fsm_cxx/fsm-common.hh"
#include "fsm_cxx/fsm-def.hh"
#include "fsm_cxx/fsm-sm.hh"

#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

#include <functional>
#include <memory>
#include <random>
#include <vector>

namespace {
  namespace hicc::dp::state::basic {
  } // namespace hicc::dp::state::basic
  namespace hicc::dp::state::bugs {
    int v = 0;
  } // namespace hicc::dp::state::bugs

  void test_state_basic() {
    using namespace hicc::dp::state::basic;
  }
} // namespace

namespace fsm_cxx::test {

namespace {

  // states

  AWESOME_MAKE_ENUM(my_state,
                    Empty,
                    Error,
                    Initial,
                    Terminated,
                    Opened,
                    Closed)

  // events

#if 1
  struct begin : public fsm_cxx::event_type<begin> {
    ~begin() override {}
    int val{9};
  };
  FSM_DEFINE_EVENT(end);
  FSM_DEFINE_EVENT(open);
  FSM_DEFINE_EVENT(close);
#else
  struct event_base {};
  struct begin : public event_base {};
  struct end : public event_base {};
  struct open : public event_base {};
  struct close : public event_base {};
#endif

  void test_state_meta() {
    // using namespace hicc::dp::state::meta;
    // using namespace hmeta;
    machine_t<my_state> m;
    using M = decltype(m);

    // @formatter:off
    // states
    m.state().set(my_state::Initial).as_initial().build();
    m.state().set(my_state::Terminated).as_terminated().build();
    m.state().set(my_state::Error).as_error().entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cerr << "          .. <error> entering" << '\n'; }).build();
    m.state().set(my_state::Opened).guard([](M::Event const &, M::Context &, M::State const &, M::Payload const &) -> bool { return true; }).guard([](M::Event const &, M::Context &, M::State const &, M::Payload const &p) -> bool { return p._ok; }).entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <opened> entering" << '\n'; }).exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <opened> exiting" << '\n'; }).build();
    m.state().set(my_state::Closed).entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed> entering" << '\n'; }).exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed> exiting" << '\n'; }).build();

    // transitions
    m.transition().set(my_state::Initial, begin{}, my_state::Closed).build();
    m.transition()
        .set(my_state::Closed, open{}, my_state::Opened)
        .guard([](M::Event const &, M::Context &, M::State const &, M::Payload const &p) -> bool { return p._ok; })
        .entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed -> opened> entering" << '\n'; })
        .exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed -> opened> exiting" << '\n'; })
        .build();
    m.transition().set(my_state::Opened, close{}, my_state::Closed).build().transition().set(my_state::Closed, end{}, my_state::Terminated).build();
    m.transition().set(my_state::Opened, end{}, my_state::Terminated).entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <T><END>" << '\n'; }).build();
    // @formatter:on

    // error states, or error state transition handler
    m.on_error([](Reason reason, M::State const &, M::Context &, M::Event const &, M::Payload const &) {
      std::cout << "          Error: reason = " << reason << '\n';
    });

    // debug log
    m.on_transition([&m](auto const &from, fsm_cxx::event_t const &ev, auto const &to, auto const &actions, auto const &payload) {
      std::printf("        [%s] -- %s --> [%s] (payload = %s)\n", m.state_to_sting(from).c_str(), ev.to_string().c_str(), m.state_to_sting(to).c_str(), to_string(payload).c_str());
      UNUSED(actions);
    });

    // processing

    m.step_by(begin{});
    if (!m.step_by(open{}, payload_t{false}))
      std::cout << "          E. cannot step to next with a false payload\n";
    m.step_by(open{});
    m.step_by(close{});
    m.step_by(open{});
    m.step_by(end{});

    std::printf("---- END OF test_state_meta() | v=%d\n\n\n", hicc::dp::state::bugs::v);
  }

  // TODO 1. hierarchical state

  AWESOME_MAKE_ENUM(calculator,
                    Empty,
                    Error,
                    Off,          // Initial state, Terminated state
                    On,           // CE/On pressed
                    ON_Op1,       // +,-,*,/
                    ON_Op2,       // 0..9
                    ON_OpResult,  // =
                    ON_OpEntered, //
                    ON_CE)

  void test_state_meta_2() {
    // using namespace hicc::dp::state::meta;
    // using namespace hmeta;
    machine_t<my_state, fsm_cxx::event_t, std::mutex> m;
    using M = decltype(m);

    std::cout << calculator::Empty << '\n';

    m.state().set(my_state::Initial).as_initial().build();
    m.state().set(my_state::Terminated).as_terminated().build();
    m.state().set(my_state::Error).as_error().entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cerr << "          .. <error> entering" << '\n'; }).build();

    m.transition().set(my_state::Initial, begin{}, my_state::Closed).build();
    m.transition().set(my_state::Closed, open{}, my_state::Opened).entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed -> opened> entering" << '\n'; }).exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed -> opened> exiting" << '\n'; }).build();
    m.transition().set(my_state::Opened, close{}, my_state::Closed).build();
    m.transition().set(my_state::Closed, end{}, my_state::Terminated).build();
    m.transition().set(my_state::Opened, end{}, my_state::Terminated).entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <T><END>" << '\n'; }).build();

    // debug log
    m.on_transition([&m](auto const &from, auto const &ev, auto const &to, auto const &actions, auto const &payload) {
      std::printf("        [%s] -- %s --> [%s] (payload: %s)\n", m.state_to_sting(from).c_str(), std::string(debug::type_name<decltype(ev)>()).c_str(), m.state_to_sting(to).c_str(), to_string(payload).c_str());
      UNUSED(actions);
    });

    // processing

    m << begin{} << open{} << close{} << open{} << end{};

    std::printf("---- END OF test_state_meta_2()\n\n\n");
  }
}
} // namespace fsm_cxx::test

namespace lambdas {
namespace {
  static void f(int n1, int n2, int n3, const int &n4, int n5) {
    std::cout << n1 << ' ' << n2 << ' ' << n3 << ' ' << n4 << ' ' << n5 << '\n';
  }

  static int g(int n1) { return n1; }

  struct Foo {
    void print_sum(int n1, int n2) {
      std::cout << n1 + n2 << '\n';
    }
    int data = 10;
  };

  static void test_lambdas() {
    {
      using namespace std::placeholders;
      std::vector<std::function<int(int)>> functions;

      std::function<int(int, int)> const foo = [](int a, int b) { return a + b; };
      std::function<int(int)> const bar = [foo](auto &&PH1) { return foo(2, std::forward<decltype(PH1)>(PH1)); };
      std::function<int(int, int)> const bar2args = [foo](auto &&PH1, auto &&PH2) { return foo(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); };

      functions.push_back(bar);
      UNUSED(bar2args);
    }

    {
      using namespace std::placeholders;
      using namespace lambdas;

      // 演示参数重排序和按引用传递
      int n = 7;
      // （ _1 与 _2 来自 std::placeholders ，并表示将来会传递给 f1 的参数）
      auto f1 = [capture0 = std::cref(n), n](auto &&PH1, auto &&PH2) { f(std::forward<decltype(PH2)>(PH2), 42, std::forward<decltype(PH1)>(PH1), capture0, n); };
      n = 10;
      f1(1, 2); // 1 为 _1 所绑定， 2 为 _2 所绑定，不使用 1001
                // 进行到 f(2, 42, 1, n, 7) 的调用

// not for c++20 and higher
#if 0
      // 嵌套 bind 子表达式共享占位符
      auto f2 = std::bind(f, _3, [](auto &&, auto &&, auto &&PH3) { return g(std::forward<decltype(PH3)>(PH3)); }, _3, 4, 5);
      f2(10, 11, 12); // 进行到 f(12, g(12), 12, 4, 5); 的调用
#endif

// not for c++20 and higher
#if 0
      // 常见使用情况：以分布绑定 RNG
      std::default_random_engine const e;
      std::uniform_int_distribution<> const d(0, 10);
      std::function<int()> const rnd = [d, e] { return d(e); }; // e 的一个副本存储于 rnd
      for (int n1 = 0; n1 < 10; ++n1)
        std::cout << rnd() << ' ';
      std::cout << '\n';
#endif

      // 绑定指向成员函数指针
      Foo foo;
      auto f3 = [ObjectPtr = &foo](auto &&PH1) { ObjectPtr->print_sum(95, std::forward<decltype(PH1)>(PH1)); };
      f3(5);

      // 绑定指向数据成员指针
      auto f4 = std::bind(&Foo::data, _1);
      std::cout << f4(foo) << '\n';

      // 智能指针亦能用于调用被引用对象的成员
      std::cout << f4(std::make_shared<Foo>(foo)) << '\n'
                << f4(std::make_unique<Foo>(foo)) << '\n';
    }
  }
}
} // namespace lambdas

namespace {

  void test_lock_guard() {
    {
      std::mutex mu;
      mu.unlock();
      std::lock_guard<std::mutex> const lock(mu);
    }
    {
      fsm_cxx::util::cool::lock_guard<std::mutex> const lock;
    }
    {
      fsm_cxx::util::cool::lock_guard<void> const lock;
    }
  }

} // namespace

int main() {

  test_lock_guard();

  UNUSED(lambdas::test_lambdas, lambdas::g);
  // lambdas::test_lambdas();

  test_state_basic();
  fsm_cxx::test::test_state_meta();

  fsm_cxx::test::test_state_meta_2();

  return 0;
}