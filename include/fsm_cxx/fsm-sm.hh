//
// Created by Hedzr Yeh on 2021/9/26.
//

#ifndef FSM_CXX_FSM_SM_HH
#define FSM_CXX_FSM_SM_HH


#include "fsm-def.hh"

#include "fsm-assert.hh"
#include "fsm-debug.hh"

#include <algorithm>
#include <functional>
#include <memory>
#include <new>
#include <random>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <atomic>
#include <condition_variable>
#include <mutex>

// #include <any>
// #include <array>
#include <chrono>
// #include <deque>
// #include <initializer_list>
// #include <list>
// #include <map>
// #include <set>
#include <unordered_map>
#include <vector>

// #include <cstdio>
// #include <cstdlib>
// #include <cstring> // std::strcmp, ...
// #include <iomanip>
// #include <iostream>
#include <string>

#include <math.h>

namespace fsm_cxx {

    enum class state_t {
        Empty,
        Initial,
        Terminated,
    };

    struct dummy_event {};

    template<typename EventT>
    struct event_t {
        EventT eo{};
        event_t() {}
        event_t(EventT const &t)
            : eo(t) {}
        event_t(EventT &&t)
            : eo(std::move(t)) {}
        event_t(event_t const &t)
            : eo(t.eo) {}
        bool operator==(EventT const &t) const { return typeid(eo) == typeid(t); }
        bool operator==(event_t const &t) const { return typeid(eo) == typeid(t.eo); }
        bool operator==(std::string const &ev_name) const { return ev_name == fsm_cxx::debug::type_name<EventT>(); }
    };

    template<typename State>
    struct context_t {
        State current;
    };
} // namespace fsm_cxx

// action_t
namespace fsm_cxx {

    template<typename State, typename Context = context_t<State>>
    class action_t {
    public:
        using FN = std::function<void(Context &ctx, State next_or_prev)>;
        ~action_t() {}
        action_t() {}
        action_t(std::nullptr_t) {}
        // explicit action_t(action_t &&f)
        //     : _f(std::move(f._f)) {}
        explicit action_t(action_t const &f)
            : _f(f._f) {}
        explicit action_t(FN &&f)
            : _f(std::move(f)) {}
        template<typename _Callable, typename... _Args,
                 std::enable_if_t<!std::is_same<std::decay_t<_Callable>, FN>::value &&
                                          !std::is_same<std::decay_t<_Callable>, action_t>::value &&
                                          !std::is_same<std::decay_t<_Callable>, std::nullopt_t>::value &&
                                          !std::is_same<std::decay_t<_Callable>, std::nullptr_t>::value,
                                  int> = 0>
        action_t(_Callable &&f, _Args &&...args) {
            using namespace std::placeholders;
            _f = fsm_cxx::util::cool::bind_tie<2>(std::forward<_Callable>(f), std::forward<_Args>(args)..., _1, _2);
        }

        void operator()(Context &ctx, State next) const {
            if (_f) _f(ctx, next);
        }

    private:
        FN _f;
    }; // class action_t

#if defined(_DEBUG)
    action_t<state_t> abc;
    action_t<state_t> abc1{nullptr};
    action_t<state_t> abc2{[](context_t<state_t> &, state_t) {}, std::placeholders::_1, std::placeholders::_2};
    action_t<state_t> abc3;

    template<typename State, typename Action = action_t<State>>
    struct moore_state_t {
        State state;
        Action entry_action;
        Action exit_action;
    };

    template<typename State, typename Char = char, typename Action = action_t<State>>
    struct mealy_state_t {
        State state;
        Action entry_action;
        Action exit_action;
        friend std::basic_istream<Char> &operator>>(std::basic_istream<Char> &is, mealy_state_t &o) {
            Char c;
            is >> c;
            UNUSED(o);
            return is;
        }
    };
#endif
} // namespace fsm_cxx

// links_t
namespace fsm_cxx { namespace detail {
    template<typename State>
    struct links_t {
        std::string event_name{};
        State to{};
        links_t() {}
        links_t(std::string const &e, State const &st)
            : event_name(e)
            , to(st) {}
        links_t(links_t const &o)
            : event_name(o.event_name)
            , to(o.to) {}
        bool operator==(links_t const &rhs) const { return event_name == rhs.event_name && to == rhs.to; }
    };
}} // namespace fsm_cxx::detail

namespace std {
    template<typename State>
    struct hash<fsm_cxx::detail::links_t<State>> {
        typedef fsm_cxx::detail::links_t<State> argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const &s) const {
            result_type h1(std::hash<int>{}(static_cast<int>(s.to)));
            hash_combine(h1, s.event_name);
            return h1;
        }
    };
} // namespace std

// actions_t
namespace fsm_cxx { namespace detail {
    template<typename State, typename Action = action_t<State>>
    struct actions_t {
        Action entry_action;
        Action exit_action;
        actions_t(Action &&entry = nullptr, Action &&exit = nullptr)
            : entry_action(std::move(entry))
            , exit_action(std::move(exit)) {}
        actions_t(actions_t const &o)
            : entry_action(o.entry_action)
            , exit_action(o.exit_action) {}
    };
}} // namespace fsm_cxx::detail

// trans_item_t
namespace fsm_cxx { namespace detail {
    template<typename State, typename Action = action_t<State>>
    struct trans_item_t {
        State to{};
        Action entry_action{nullptr};
        Action exit_action{nullptr};
        trans_item_t(State const &st = State{}, Action &&entry = nullptr, Action &&exit = nullptr)
            : to(st)
            , entry_action(std::move(entry))
            , exit_action(std::move(exit)) {}
        trans_item_t(trans_item_t const &o)
            : to(o.to)
            , entry_action(o.entry_action)
            , exit_action(o.exit_action) {}
    };
}} // namespace fsm_cxx::detail

namespace fsm_cxx {

    template<typename State, typename Action = action_t<State>>
    struct transition_t_ {
        using First = detail::links_t<State>;
        using Second = detail::actions_t<State, Action>;
        using Maps = std::unordered_map<First, Second>;
        Maps m_;

        transition_t_() {}
        ~transition_t_() {}

        template<typename Event>
        transition_t_(Event const &, State const &to, Action &&entry = nullptr, Action &&exit = nullptr) {
            std::string event_name{fsm_cxx::debug::type_name<Event>()};
            // First k{event_name, to};
            // Second v(std::move(entry), std::move(exit));
            // m_[k] = v;
            // m_.insert({First{event_name, to}, Second{std::move(entry), std::move(exit)}});
            // m_.emplace(k, v);
            m_.emplace(std::move(First{event_name, to}), std::move(Second{std::move(entry), std::move(exit)}));
        }
        transition_t_(std::string const &event_name, State const &to, Action &&entry = nullptr, Action &&exit = nullptr) {
            m_.emplace(std::move(First{event_name, to}), std::move(Second{std::move(entry), std::move(exit)}));
        }
        void add(transition_t_ &&t) {
            for (auto &[k, v] : t.m_)
                m_.emplace(k, v);
        }
        void add(transition_t_ const &t) {
            for (auto const &[k, v] : t.m_)
                m_.insert({k, v});
        }
        std::tuple<bool, State, Second const &> get(std::string const &event_name) const {
            for (auto const &[k, v] : m_) {
                if (k.event_name == event_name)
                    return std::tuple<bool, State, Second const &>{true, k.to, v};
            }
            return std::tuple<bool, State, Second const &>{false, State{}, Second{}};
        }
    };

    template<typename State, typename Action = action_t<State>>
    struct transition_t {
        using First = std::string;
        using Second = detail::trans_item_t<State, Action>;
        using Maps = std::unordered_map<First, Second>;
        Maps m_;

        transition_t() {}
        ~transition_t() {}

        template<typename Event>
        transition_t(Event const &, State const &to, Action &&entry = nullptr, Action &&exit = nullptr) {
            std::string event_name{fsm_cxx::debug::type_name<Event>()};
            m_.emplace(std::move(First{event_name}), std::move(Second{to, std::move(entry), std::move(exit)}));
        }
        transition_t(std::string const &event_name, State const &to, Action &&entry = nullptr, Action &&exit = nullptr) {
            m_.emplace(std::move(First{event_name}), std::move(Second{to, std::move(entry), std::move(exit)}));
        }
        void add(transition_t &&t) {
            for (auto &[k, v] : t.m_)
                m_.emplace(k, v);
        }
        void add(transition_t const &t) {
            for (auto const &[k, v] : t.m_)
                m_.insert({k, v});
        }
        std::tuple<bool, Second const &> get(std::string const &event_name) const {
            auto it = m_.find(event_name);
            if(it!=m_.end())
                return std::tuple<bool, Second const&>{true, it->second};
            return std::tuple<bool, Second const &>{false, Second{}};
        }
    };

    template<typename StateT = state_t,
             typename ContextT = context_t<StateT>,
             typename ActionT = action_t<StateT, ContextT>,
             typename CharT = char,
             typename InT = std::basic_istream<CharT>>
    class machine_t final {
    public:
        machine_t() {}
        ~machine_t() {}

        using State = StateT;
        using Action = ActionT;
        using Context = ContextT;
        using Transition = transition_t<StateT, ActionT>;
        using TransitionTable = std::unordered_map<StateT, Transition>;
        using on_action = std::function<void(State const &, std::string const &, State const &, typename Transition::Second const &)>;

    public:
        machine_t &initial(State st) {
            _initial = _ctx.current = st;
            return (*this);
        }
        machine_t &terminated(State st) {
            _terminated = st;
            return (*this);
        }
        machine_t &error(State st) {
            _error = st;
            return (*this);
        }
        machine_t &reset() {
            _ctx.current = _initial;
            return (*this);
        }
        template<typename Event = dummy_event>
        machine_t &transition(State from, Event const &e, State to, Action &&entry_action = nullptr, Action &&exit_action = nullptr) {
            // event_t<Event>(e)
            Transition t{e, to, std::move(entry_action), std::move(exit_action)};
            return transition(from, std::move(t));
        }
        machine_t &transition(State from, Transition &&transition) {
            if (auto it = _trans_tbl.find(from); it == _trans_tbl.end())
                _trans_tbl.emplace(from, std::move(transition));
            else
                it->second.add(std::move(transition));
            return (*this);
        }
        machine_t &transition(State from, Transition const &transition) {
            if (auto it = _trans_tbl.find(from); it == _trans_tbl.end())
                _trans_tbl.insert({from, transition});
            else
                it->second.add(transition);
            return (*this);
        }
        machine_t &on_action_for_debug(on_action &&fn) {
            _on_action = fn;
            return (*this);
        }

    public:
        template<typename Event = dummy_event>
        void step_by(Event const &) {
            std::string event_name{fsm_cxx::debug::type_name<Event>()};
            if (auto it = _trans_tbl.find(_ctx.current); it != _trans_tbl.end()) {
                auto &tr = it->second;
                auto [ok, itr] = tr.get(event_name);
                if (ok) {
                    auto from = _ctx.current;
                    itr.exit_action(_ctx, from);
                    itr.entry_action(_ctx, itr.to);
                    _on_action(_ctx.current, event_name, itr.to, itr);
                    // UNUSED(actions);
                    // fsm_debug("        [%s] -- %s --> [%s]", state_to_sting(_ctx.current).c_str(), event_name.c_str(), state_to_sting(to).c_str());
                    _ctx.current = itr.to;
                }
            }
        }

    public:
        static std::string state_to_sting(State state) { return shorten(fsm_cxx::to_string(state)); }

    protected:
        static std::string shorten(std::string const &s) {
            auto pos = s.rfind("::");
            return pos == std::string::npos ? s : s.substr(pos + 2);
        }

        friend std::basic_istream<CharT> &operator>>(std::basic_istream<CharT> &is, machine_t &o) {
            CharT c;
            is >> c; // TODO process the input stream (is >> c) and convert it to event to trigger
            o.step_by(c);
            return is;
        }

    private:
        Context _ctx{};
        State _initial{}, _terminated{}, _error{};
        TransitionTable _trans_tbl{};
        on_action _on_action{};
    }; // class machine_t

} // namespace fsm_cxx

#endif //FSM_CXX_FSM_SM_HH
