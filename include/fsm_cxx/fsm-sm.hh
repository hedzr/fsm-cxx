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

    enum class state_e {
        Empty,
        Initial,
        Terminated,
    };

    template<typename T, typename MutexT = void>
    struct state_t;
} // namespace fsm_cxx

// event_t
namespace fsm_cxx {
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

        friend std::ostream &operator<<(std::ostream &os, event_t const &o) {
            return os << o.eo;
        }
    };

    struct payload_t {
        ~payload_t() {}
        friend std::ostream &operator<<(std::ostream &os, payload_t const &) { return os << "a payload"; }
    };
} // namespace fsm_cxx

// context_t
namespace fsm_cxx {
    template<typename State, typename MutexT = void>
    struct context_t {
        using lock_guard_t = util::cool::lock_guard<MutexT>;

        State current;
    };
} // namespace fsm_cxx

// state_t
namespace fsm_cxx {

    template<typename T, typename MutexT>
    struct state_t {
        using State = state_t<T, MutexT>;
        using Context = context_t<State, MutexT>;
        // template<typename EventT>
        // using FN = std::function<void(event_t<EventT> const &ev, Context &ctx, State const &next_or_prev)>;

        T t{};
        // template<typename EventT>
        // FN enter, exit;

        state_t() {}
        virtual ~state_t() {}
        state_t(T t_)
            : t(t_) {}
        state_t(state_t const &o)
            : t(o.t) {}

        state_t &operator=(T t_) {
            t = t_;
            return (*this);
        }

        bool operator==(state_t const &o) const { return t == o.t; }
        bool operator==(T const &o) const { return t == o; }
        friend std::ostream &operator<<(std::ostream &os, state_t const &o) { return os << o.t; }

        // template<typename EventT>
        // void on_enter(event_t<EventT> const &ev, Context &ctx, State const &next_or_prev) { UNUSED(ev, ctx, next_or_prev); }
        // template<typename EventT>
        // void on_exit(event_t<EventT> const &ev, Context &ctx, State const &next_or_prev) { UNUSED(ev, ctx, next_or_prev); }
    };

} // namespace fsm_cxx

// action_t
namespace fsm_cxx {

    template<typename S,
             typename EventT = dummy_event,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, MutexT>>
    class action_t {
    public:
        using State = StateT;
        using Context = ContextT;
        using Payload = PayloadT;
        using FN = std::function<void(EventT const &ev, Context &ctx, State const &next_or_prev, Payload const &payload)>;

        action_t() {}
        ~action_t() {}
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
            _f = fsm_cxx::util::cool::bind_tie<4>(std::forward<_Callable>(f), std::forward<_Args>(args)..., _1, _2, _3, _4);
        }

        void operator()(EventT const &ev, Context &ctx, State const &next_or_prev, Payload const &payload) const {
            if (_f) _f(ev, ctx, next_or_prev, payload);
        }

        operator bool() const { return bool(_f); }

    private:
        FN _f;
    }; // class action_t

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

// hash for state_t/links_t
namespace std {
    template<typename State, typename MutexT>
    struct hash<fsm_cxx::state_t<State, MutexT>> {
        typedef fsm_cxx::state_t<State, MutexT> argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const &s) const {
            result_type h1(std::hash<int>{}(static_cast<int>(s.t)));
            return h1;
        }
    };

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
    template<typename S,
             typename EventT = dummy_event,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, MutexT>,
             typename ActionT = action_t<S, EventT, MutexT, PayloadT, StateT, ContextT>>
    struct actions_t {
        ActionT entry_action;
        ActionT exit_action;
        actions_t(ActionT &&entry = nullptr, ActionT &&exit = nullptr)
            : entry_action(std::move(entry))
            , exit_action(std::move(exit)) {}
        actions_t(actions_t const &o)
            : entry_action(o.entry_action)
            , exit_action(o.exit_action) {}
        bool valid() const { return entry_action || exit_action; }
    };
}} // namespace fsm_cxx::detail

// trans_item_t
namespace fsm_cxx { namespace detail {
    template<typename S,
             typename EventT = dummy_event,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, MutexT>,
             typename ActionT = action_t<S, EventT, MutexT, PayloadT, StateT, ContextT>>
    struct trans_item_t {
        using State = StateT;
        using Context = ContextT;
        using Payload = PayloadT;
        using Action = ActionT;

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

    template<typename S,
             typename EventT = dummy_event,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, MutexT>,
             typename ActionT = action_t<S, EventT, MutexT, PayloadT, StateT, ContextT>>
    struct transition_t {
        using Event = EventT;
        using State = StateT;
        using Context = ContextT;
        using Payload = PayloadT;
        using Action = ActionT;
        using First = std::string;
        using Second = detail::trans_item_t<S, EventT, MutexT, PayloadT, StateT, ContextT, ActionT>;
        using Maps = std::unordered_map<First, Second>;

        Maps m_;

        transition_t() {}
        ~transition_t() {}

        // template<typename Event>
        transition_t(Event const &, S const &to, ActionT &&entry = nullptr, ActionT &&exit = nullptr) {
            std::string event_name{fsm_cxx::debug::type_name<Event>()};
            m_.emplace(std::move(First{event_name}), std::move(Second{StateT{to}, std::move(entry), std::move(exit)}));
        }
        // template<typename Event>
        transition_t(Event const &, StateT const &to, ActionT &&entry = nullptr, ActionT &&exit = nullptr) {
            std::string event_name{fsm_cxx::debug::type_name<Event>()};
            m_.emplace(std::move(First{event_name}), std::move(Second{to, std::move(entry), std::move(exit)}));
        }
        transition_t(std::string const &event_name, StateT const &to, ActionT &&entry = nullptr, ActionT &&exit = nullptr) {
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
        std::tuple<bool, Second const &> get(std::string const &event_name) const { return _get(event_name); }
        std::tuple<bool, Second &> get(std::string const &event_name) { return _get(event_name); }
        std::tuple<bool, Second &> _get(std::string const &event_name) {
            auto it = m_.find(event_name);
            if (it != m_.end())
                return std::tuple<bool, Second &>{true, it->second};
            static Second s2{};
            return std::tuple<bool, Second &>{false, s2};
        }
    };

    template<typename S,
             typename EventT = dummy_event,
             typename MutexT = void, // or std::mutex
             typename PayloadT = payload_t,
             typename StateT = state_t<S>,
             typename ContextT = context_t<StateT, MutexT>,
             typename ActionT = action_t<S, EventT, MutexT, PayloadT, StateT, ContextT>,
             typename CharT = char,
             typename InT = std::basic_istream<CharT>>
    class machine_t final {
    public:
        machine_t() {}
        ~machine_t() {}
        machine_t(machine_t const &) = default;
        machine_t &operator=(machine_t &) = delete;

        using Event = EventT;
        using State = StateT;
        using Context = ContextT;
        using Payload = PayloadT;
        using Action = ActionT;
        using Actions = detail::actions_t<S, EventT, MutexT, PayloadT, StateT, ContextT, ActionT>;
        using Transition = transition_t<S, EventT, MutexT, PayloadT, StateT, ContextT, ActionT>;
        using TransitionTable = std::unordered_map<StateT, Transition>;
        using OnAction = std::function<void(StateT const &, std::string const &, StateT const &, typename Transition::Second const &, Payload const &)>;
        using StateActions = std::unordered_map<StateT, Actions>;
        using lock_guard_t = util::cool::lock_guard<MutexT>;

    public:
        machine_t &initial(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            _initial = _ctx.current = st;
            return state(st, std::move(entry_action), std::move(exit_action));
        }
        machine_t &terminated(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            _terminated = st;
            return state(st, std::move(entry_action), std::move(exit_action));
        }
        machine_t &error(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            _error = st;
            return state(st, std::move(entry_action), std::move(exit_action));
        }

        machine_t &state(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            Actions actions{std::move(entry_action), std::move(exit_action)};
            if (actions.valid())
                _state_actions.emplace(StateT{st}, std::move(actions));
            return (*this);
        }

        machine_t &reset() {
            _ctx.current = _initial;
            return (*this);
        }

        // template<typename Event = dummy_event>
        machine_t &transition(S from, Event const &e, S to, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            // event_t<Event>(e)
            Transition t{e, to, std::move(entry_action), std::move(exit_action)};
            return transition(from, std::move(t));
        }
        machine_t &transition(S from, Transition &&transition) {
            if (auto it = _trans_tbl.find(State{from}); it == _trans_tbl.end())
                _trans_tbl.emplace(State{from}, std::move(transition));
            else
                it->second.add(std::move(transition));
            return (*this);
        }
        machine_t &transition(S from, Transition const &transition) {
            if (auto it = _trans_tbl.find(State{from}); it == _trans_tbl.end())
                _trans_tbl.insert({State{from}, transition});
            else
                it->second.add(transition);
            return (*this);
        }
        machine_t &on_action_for_debug(OnAction &&fn) {
            _on_action = fn;
            return (*this);
        }

    public:
        void step_by(Event const &ev) { step_by(ev, Payload{}); }
        void step_by(Event const &ev, Payload const &payload) {
            std::string event_name{fsm_cxx::debug::type_name<Event>()};

            lock_guard_t locker;
            if (auto it = _trans_tbl.find(_ctx.current); it != _trans_tbl.end()) {
                auto &tr = it->second;
                auto [ok, itr] = tr.get(event_name);
                if (ok) {
                    auto &from = _ctx.current;

                    locker.unlock();

                    auto leave = _state_actions.find(from);
                    itr.exit_action(ev, _ctx, from, payload);
                    if (leave != _state_actions.end())
                        leave->second.exit_action(ev, _ctx, itr.to, payload);

                    _ctx.current = itr.to;
                    if (_on_action) _on_action(from, event_name, itr.to, itr, payload);

                    auto enter = _state_actions.find(itr.to);
                    itr.entry_action(ev, _ctx, itr.to, payload);
                    if (enter != _state_actions.end())
                        enter->second.entry_action(ev, _ctx, from, payload);

                    // UNUSED(actions);
                    // fsm_debug("        [%s] -- %s --> [%s]", state_to_sting(_ctx.current).c_str(), event_name.c_str(), state_to_sting(to).c_str());
                }
            }
        }

    public:
        static std::string state_to_sting(StateT const &state) { return shorten(to_string(state)); }
        static std::string state_to_sting(S const &state) { return shorten(to_string(state)); }

    protected:
        static std::string shorten(std::string const &s) {
            auto pos = s.rfind("::");
            return pos == std::string::npos ? s : s.substr(pos + 2);
        }

        friend std::basic_istream<CharT> &operator>>(std::basic_istream<CharT> &is, machine_t &o) {
            CharT c;
            is >> c; // TODO process the input stream (is >> c) and convert it to event and trigger
            o.step_by(c);
            return is;
        }

    private:
        ContextT _ctx{};
        StateT _initial{}, _terminated{}, _error{};
        TransitionTable _trans_tbl{};
        OnAction _on_action{};
        StateActions _state_actions{};
    }; // class machine_t

} // namespace fsm_cxx

#endif //FSM_CXX_FSM_SM_HH