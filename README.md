# fsm-cxx

![CMake Build Matrix](https://github.com/hedzr/fsm-cxx/workflows/CMake%20Build%20Matrix/badge.svg) <!-- 
![CMake Build Matrix](https://github.com/hedzr/fsm-cxx/workflows/CMake%20Build%20Matrix/badge.svg?event=release) 
--> [![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/hedzr/fsm-cxx.svg?label=release)](https://github.com/hedzr/fsm-cxx/releases)

`fsm-cxx` is a finite state machina library for C++17, header-only, light-weight but full-featured, and designed for easy binding and friendly programmatic interface.


## Features

- Entry/exit actions
- Event actions, guards
- Transition actions
- Transition conditions (input action)
- Event payload (classes)
- Thread Safe (`safe_machine_t<>`)
- [ ] Inheritance of states and action functions
- [ ] Documentations (NOT YET)
- [ ] Examples (NOT YET)

---

<!--
Statechart features
Hierarchical states
Entry and exit actions
Internal transitions
Transition actions
Transition guards (conditions)
State history
Event deferring
Orthogonal regions
Statechart extensions
Optional event priority
Optional common base for states and easy definition of dispatching common interface calls to current state
Pushdown automaton
Compile-time checks
Thread safety
Exception safety
No vtables (unless common base feature is used)
Header only
Relatively fast compile time
No external dependencies except STL
-->

## Usages

Here is a simple state machine:

```cpp
#include <fsm_cxx.hh>
namespace fsm_cxx { namespace test {

    // states

    AWESOME_MAKE_ENUM(my_state,
                      Empty,
                      Error,
                      Initial,
                      Terminated,
                      Opened,
                      Closed)

    // event

    // Or:
    // FSM_DEFINE_EVENT_BEGIN(begin)
    //     int val{9}
    // FSM_DEFINE_EVENT_END()
    struct begin : public fsm_cxx::event_type<begin> {
        virtual ~begin() {}
        int val{9};
    };
    struct end : public fsm_cxx::event_type<end> {
        virtual ~end() {}
    };
    struct open : public fsm_cxx::event_type<open> {
        virtual ~open() {}
    };
    // Or:
    // FSM_DEFINE_EVENT(close)
    struct close : public fsm_cxx::event_type<close> {
        virtual ~close() {}
    };

    void test_state_meta() {
        fsm_cxx::machine_t<my_state> m;
        using M = decltype(m);

        // @formatter:off
        // states
        m.state().set(my_state::Initial).as_initial().build();
        m.state().set(my_state::Terminated).as_terminated().build();
        m.state().set(my_state::Error).as_error()
                .entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cerr << "          .. <error> entering" << '\n'; })
                .build();
        m.state().set(my_state::Opened)
                .guard([](M::Event const &, M::Context &, M::State const &, M::Payload const &) -> bool { return true; })
                .guard([](M::Event const &, M::Context &, M::State const &, M::Payload const &p) -> bool { return p._ok; })
                .entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <opened> entering" << '\n'; })
                .exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <opened> exiting" << '\n'; })
                .build();
        m.state().set(my_state::Closed)
                .entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed> entering" << '\n'; })
                .exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed> exiting" << '\n'; })
                .build();

        // transitions
        m.transition().set(my_state::Initial, begin{}, my_state::Closed).build();
        m.transition()
                .set(my_state::Closed, open{}, my_state::Opened)
                .guard([](M::Event const &, M::Context &, M::State const &, M::Payload const &p) -> bool { return p._ok; })
                .entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed -> opened> entering" << '\n'; })
                .exit_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <closed -> opened> exiting" << '\n'; })
                .build();
        m.transition().set(my_state::Opened, close{}, my_state::Closed).build()
                .transition().set(my_state::Closed, end{}, my_state::Terminated).build();
        m.transition().set(my_state::Opened, end{}, my_state::Terminated)
                .entry_action([](M::Event const &, M::Context &, M::State const &, M::Payload const &) { std::cout << "          .. <T><END>" << '\n'; })
                .build();
        // @formatter:on

        m.on_error([](fsm_cxx::Reason reason, M::State const &, M::Context &, M::Event const &, M::Payload const &) {
            std::cout << "          Error: reason = " << reason << '\n';
        });

        // debug log
        m.on_transition([&m](auto const &from, fsm_cxx::event_t const &ev, auto const &to, auto const &actions, auto const &payload) {
            std::printf("        [%s] -- %s --> [%s] (payload = %s)\n", m.state_to_sting(from).c_str(), ev.to_string().c_str(), m.state_to_sting(to).c_str(), to_string(payload).c_str());
            UNUSED(actions);
        });

        // processing

        m.step_by(begin{});
        if (!m.step_by(open{}, fsm_cxx::payload_t{false}))
            std::cout << "          E. cannot step to next with a false payload\n";
        m.step_by(open{});
        m.step_by(close{});
        m.step_by(open{});
        m.step_by(end{});

        std::printf("---- END OF test_state_meta()\n\n\n");
    }
}

int main() {
    fsm_cxx::test::test_state_meta();
    return 0;
}
```



## Build Options

### Build with CMake

> 1. gcc 10+: passed
> 2. clang 12+: passed
> 3. msvc build tool 16.7.2, 16.8.5 (VS2019 or Build Tool) passed

ninja is optional for faster building.

```bash
# configure
cmake -S . -B build/ -G Ninja
# build
cmake --build build/
# install
cmake --install build/
# Or:cmake --build build/ --target install
#
# Sometimes sudo it:
#   sudo cmake --build build/ --target install
# Or:
#   cmake --install build/ --prefix ./install --strip
#   sudo cp -R ./install/include/* /usr/local/include/
#   sudo cp -R ./install/lib/cmake/fsm_cxx /usr/local/lib/cmake/
```


### Other CMake Options

1. `FSM_CXX_BUILD_TESTS_EXAMPLES`=OFF
2. `FSM_CXX_BUILD_DOCS`=OFF
3. ...


## Thanks to JODL

Thanks to [JetBrains](https://www.jetbrains.com/?from=fsm-cxx) for donating product licenses to help develop **fsm-cxx** [![jetbrains](https://gist.githubusercontent.com/hedzr/447849cb44138885e75fe46f1e35b4a0/raw/bedfe6923510405ade4c034c5c5085487532dee4/jetbrains-variant-4.svg)](https://www.jetbrains.com/?from=hedzr/fsm-cxx)


## LICENSE

MIT


## 🔚
