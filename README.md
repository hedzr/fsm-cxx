# fsm-cxx

![CMake Build Matrix](https://github.com/hedzr/fsm-cxx/workflows/CMake%20Build%20Matrix/badge.svg) <!-- 
![CMake Build Matrix](https://github.com/hedzr/fsm-cxx/workflows/CMake%20Build%20Matrix/badge.svg?event=release) 
--> [![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/hedzr/fsm-cxx.svg?label=release)](https://github.com/hedzr/fsm-cxx/releases)

`fsm-cxx` is a finite state machina library for C++17, a header-only library, and designed for easy binding and friendly programmatic interface.

## Features

- Entry/exit actions
- Event actions
- Transition functions
- Transition conditions (input action)
- Event payload (classes)
- Inheritance of states and action functions

---

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


## For Developer



### Build

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
