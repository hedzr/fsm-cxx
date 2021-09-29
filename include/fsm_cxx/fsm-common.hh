//
// Created by Hedzr Yeh on 2021/9/26.
//

#ifndef _PRIVATE_VAR_FOLDERS_0K_1RQY3K4X7_5B_73SW5PY2BW00000GN_T_CLION_CLANG_TIDY_FSM_COMMON_HH
#define _PRIVATE_VAR_FOLDERS_0K_1RQY3K4X7_5B_73SW5PY2BW00000GN_T_CLION_CLANG_TIDY_FSM_COMMON_HH

#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

// ------------------- cool::lock_guard
namespace fsm_cxx::util::cool {
    template<typename _Mutex>
    class lock_guard {
    public:
        _Mutex _m;
        lock_guard() { _m.lock(); }
        ~lock_guard() { _m.unlock(); }
        void lock() { _m.lock(); }
        void unlock() { _m.unlock(); }
    };

    template<>
    class lock_guard<void> {
    public:
        lock_guard() {}
        ~lock_guard() {}
        void lock() {}
        void unlock() {}
    };
} // namespace fsm_cxx::util::cool

// ------------------- cool::bind_tie
namespace fsm_cxx::util::cool {

    template<typename _Callable, typename... _Args>
    auto bind(_Callable &&f, _Args &&...args) {
        return std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...);
    }

    template<typename Function, typename Tuple, size_t... I>
    auto bind_N(Function &&f, Tuple &&t, std::index_sequence<I...>) {
        return std::bind(f, std::get<I>(t)...);
    }
    template<int N, typename Function, typename Tuple>
    auto bind_N(Function &&f, Tuple &&t) {
        // static constexpr auto size = std::tuple_size<Tuple>::value;
        return bind_N(f, t, std::make_index_sequence<N>{});
    }

    template<int N, typename _Callable, typename... _Args,
             std::enable_if_t<!std::is_member_function_pointer_v<_Callable>, bool> = true>
    auto bind_tie(_Callable &&f, _Args &&...args) {
        return bind_N<N>(f, std::make_tuple(args...));
    }

    template<typename Function, typename _Instance, typename Tuple, size_t... I>
    auto bind_N_mem(Function &&f, _Instance &&ii, Tuple &&t, std::index_sequence<I...>) {
        return std::bind(f, ii, std::get<I>(t)...);
    }
    template<int N, typename Function, typename _Instance, typename Tuple>
    auto bind_N_mem(Function &&f, _Instance &&ii, Tuple &&t) {
        return bind_N_mem(f, ii, t, std::make_index_sequence<N>{});
    }

    template<int N, typename _Callable, typename _Instance, typename... _Args,
             std::enable_if_t<std::is_member_function_pointer_v<_Callable>, bool> = true>
    auto bind_tie_mem(_Callable &&f, _Instance &&ii, _Args &&...args) {
        return bind_N_mem<N>(f, ii, std::make_tuple(args...));
    }
    template<int N, typename _Callable, typename... _Args,
             std::enable_if_t<std::is_member_function_pointer_v<_Callable>, bool> = true>
    auto bind_tie(_Callable &&f, _Args &&...args) {
        return bind_tie_mem<N>(std::forward<_Callable>(f), std::forward<_Args>(args)...);
    }

} // namespace fsm_cxx::util::cool

// ------------------- fsm_cxx::to_string
namespace fsm_cxx {

    template<typename T>
    inline std::string to_string(T const &t) {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }

    template<typename T>
    inline std::string to_string(std::unique_ptr<T> &t) {
        std::stringstream ss;
        ss << *t.get();
        return ss.str();
    }
    template<typename T>
    inline std::string to_string(std::shared_ptr<T> &t) {
        std::stringstream ss;
        ss << *t.get();
        return ss.str();
    }
    template<typename T>
    inline std::string to_string(std::weak_ptr<T> &t) {
        std::stringstream ss;
        ss << *t.get();
        return ss.str();
    }

} // namespace fsm_cxx

#endif // _PRIVATE_VAR_FOLDERS_0K_1RQY3K4X7_5B_73SW5PY2BW00000GN_T_CLION_CLANG_TIDY_FSM_COMMON_HH
