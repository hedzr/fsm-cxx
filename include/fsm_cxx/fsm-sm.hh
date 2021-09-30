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


// ----------------------------- forward declarations
namespace fsm_cxx {

    enum class state_e {
        Empty,
        Initial,
        Terminated,
    };

    template<typename T, typename MutexT = void>
    struct state_t;

} // namespace fsm_cxx

// ----------------------------- event_t, payload_t
namespace fsm_cxx {
    // struct dummy_event {};

    struct event_t {
        virtual ~event_t() {}
        virtual std::string to_string() const { return ""; }
    };
    template<typename T>
    struct event_type : public event_t {
        virtual ~event_type() {}
        std::string to_string() const { return shorten(std::string(debug::type_name<T>())); }
        static std::string shorten(std::string const &s) {
            auto pos = s.rfind("::");
            return pos == std::string::npos ? s : s.substr(pos + 2);
        }
    };

    // template<typename EventT>
    // struct event_t {
    //     EventT eo{};
    //
    //     event_t() {}
    //     event_t(EventT const &t)
    //         : eo(t) {}
    //     event_t(EventT &&t)
    //         : eo(std::move(t)) {}
    //     event_t(event_t const &t)
    //         : eo(t.eo) {}
    //     ~event_t() {}
    //
    //     bool operator==(EventT const &t) const { return typeid(eo) == typeid(t); }
    //     bool operator==(event_t const &t) const { return typeid(eo) == typeid(t.eo); }
    //     bool operator==(std::string const &ev_name) const { return ev_name == fsm_cxx::debug::type_name<EventT>(); }
    //
    //     friend std::ostream &operator<<(std::ostream &os, event_t const &o) {
    //         return os << o.eo;
    //     }
    // };

    struct payload_t {
        payload_t(bool ok_ = true)
            : _ok(ok_) {}
        virtual ~payload_t() {}
        virtual std::string to_string() const { return "a payload"; }
        friend std::ostream &operator<<(std::ostream &os, payload_t const &o) { return os << o.to_string(); }
        bool _ok;
    };
} // namespace fsm_cxx

#define FSM_DEFINE_EVENT(n)                    \
    struct n : public fsm_cxx::event_type<n> { \
        virtual ~n() {}                        \
    }

// ----------------------------- context_t
namespace fsm_cxx {
    template<typename State,
             typename EventT = event_t,
             typename MutexT = void,
             typename PayloadT = payload_t>
    struct context_t {
        // while you're extending from context_t, take a
        // little concerns to lock_guard_t for thread-safety
        using lock_guard_t = util::cool::lock_guard<MutexT>;
        // using Event = event_t<EventT>;
        using Payload = PayloadT;
        using Context = context_t<State, EventT, MutexT, Payload>;
        using Pred = std::function<bool(EventT const &, Context &, State const &, Payload const &)>;
        using Preds = std::vector<Pred>;
        using First = State;
        using Second = Preds;
        using Guards = std::unordered_map<First, Second>;

        /**
         * @brief reset the context to initial state
         * @param t 
         */
        void reset(State const &t, bool clear_guards = false) {
            _current = t;
            if (clear_guards)
                _guards.clear();
        }

        /**
         * @brief verify all guards for the target state
         * @param to the target state
         * @param ev the event
         * @param payload the payload
         * @return true if the target state can be transit to, else false
         */
        bool verify(State const &to, EventT const &ev, PayloadT const &payload) {
            auto it = _guards.find(to);
            if (it == _guards.end()) {
                return true;
            }

            for (auto itx = it->second.begin(); itx != it->second.end(); itx++) {
                auto &fn = *(itx);
                if (!fn(ev, *this, to, payload))
                    return false;
            }
            return true;
        }

        void current(State const &s) {
            lock_guard_t l;
            _current = s;
        }
        State const &current() const { return _current; }
        State safe_current() const {
            State tmp;
            {
                lock_guard_t l;
                tmp = _current;
            }
            return tmp;
        }

        /**
         * @brief add transition guard/condition into context
         * @tparam _Callable 
         * @tparam _Args 
         * @param st state which is target of a transition
         * @param f a function has prototype: bool(EventT const &, Context &, State const &, PayloadT const &)
         * @param args 
         */
        template<typename _Callable, typename... _Args>
        void add_guard(State const &st, _Callable &&f, _Args &&...args) {
            using namespace std::placeholders;
            auto fn = fsm_cxx::util::cool::bind_tie<4>(std::forward<_Callable>(f), std::forward<_Args>(args)..., _1, _2, _3, _4, _5);

            auto it = _guards.find(st);
            if (it == _guards.end()) {
                Second second;
                second.push_back(fn);
                _guards.insert({st, second});
                return;
            }
            it->second.push_back(fn);
        }

    private:
        State _current{};
        Guards _guards{};
    };
} // namespace fsm_cxx

// ----------------------------- state_t
namespace fsm_cxx {

    template<typename T, typename MutexT>
    struct state_t {
        using State = state_t<T, MutexT>;
        // using Context = context_t<State, MutexT>;

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
        state_t &operator=(state_t const &t_) {
            t = t_.t;
            return (*this);
        }

        bool operator==(state_t const &o) const { return t == o.t; }
        bool operator==(T const &o) const { return t == o; }
        bool operator==(T o) const { return t == o; }
        friend std::ostream &operator<<(std::ostream &os, state_t const &o) { return os << o.t; }
    };

} // namespace fsm_cxx

// ----------------------------- action_t
namespace fsm_cxx {

    template<typename S,
             typename EventT = event_t,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, EventT, MutexT, PayloadT>>
    class action_t {
    public:
        using State = StateT;
        // using Event = event_t<EventT>;
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
            constexpr auto count = 4;
            _f = fsm_cxx::util::cool::bind_tie<count>(std::forward<_Callable>(f), std::forward<_Args>(args)..., _1, _2, _3, _4);
        }

        template<typename _Callable, typename... _Args>
        void update(_Callable &&f, _Args &&...args) {
            using namespace std::placeholders;
            constexpr auto count = 4;
            _f = fsm_cxx::util::cool::bind_tie<count>(std::forward<_Callable>(f), std::forward<_Args>(args)..., _1, _2, _3, _4);
        }

        /**
         * @brief forward the event and state transition
         * @param ev the event entity
         * @param ctx context of the state machine, including the transition guards
         * @param next_or_prev prev state for entry_action; or next state for exit_action
         * @param payload 
         */
        void operator()(EventT const &ev, Context &ctx, State const &next_or_prev, Payload const &payload) const {
            if (_f) _f(ev, ctx, next_or_prev, payload);
        }

        operator bool() const { return bool(_f); }

    private:
        FN _f;
    }; // class action_t

} // namespace fsm_cxx

// ----------------------------- links_t
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

// ----------------------------- hash for state_t/links_t
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

// ----------------------------- actions_t
namespace fsm_cxx { namespace detail {
    template<typename S,
             typename EventT = event_t,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, EventT, MutexT, PayloadT>,
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

// ----------------------------- trans_item_t
namespace fsm_cxx { namespace detail {
    template<typename S,
             typename EventT = event_t,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, EventT, MutexT, PayloadT>,
             typename ActionT = action_t<S, EventT, MutexT, PayloadT, StateT, ContextT>>
    struct trans_item_t {
        using State = StateT;
        using Context = ContextT;
        using Payload = PayloadT;
        using Action = ActionT;
        using Guard = std::function<bool(EventT const &, Context &, StateT const &, Payload const &)>;

        Guard pred{nullptr}; // Transition Guard here
        State to{};
        Action entry_action{nullptr};
        Action exit_action{nullptr};

        bool verify(EventT const &ev, Context &c, Payload const &p) const {
            if (pred) return pred(ev, c, to, p);
            return true;
        }

        trans_item_t(State const &st = State{}, Guard &&p = nullptr, Action &&entry = nullptr, Action &&exit = nullptr)
            : pred(p)
            , to(st)
            , entry_action(std::move(entry))
            , exit_action(std::move(exit)) {}
        trans_item_t(trans_item_t const &o)
            : pred(o.pred)
            , to(o.to)
            , entry_action(o.entry_action)
            , exit_action(o.exit_action) {}
    };
}} // namespace fsm_cxx::detail

// ----------------------------- transition_t
namespace fsm_cxx {

    template<typename S,
             typename EventT = event_t,
             typename MutexT = void,
             typename PayloadT = payload_t,
             typename StateT = state_t<S, MutexT>,
             typename ContextT = context_t<StateT, EventT, MutexT, PayloadT>,
             typename ActionT = action_t<S, EventT, MutexT, PayloadT, StateT, ContextT>>
    struct transition_t {
        using Event = EventT;
        using State = StateT;
        using Context = ContextT;
        using Payload = PayloadT;
        using Action = ActionT;
        using First = std::string; // event_name
        using Item = detail::trans_item_t<S, EventT, MutexT, PayloadT, StateT, ContextT, ActionT>;
        using Second = std::vector<Item>;
        using Maps = std::unordered_map<First, Second>;
        using Guard = typename Item::Guard;

        Maps m_;

        transition_t() {}
        ~transition_t() {}

        template<typename Evt,
                 std::enable_if_t<std::is_base_of<Event, std::decay_t<Evt>>::value && !std::is_same<Evt, std::string>::value, bool> = true>
        transition_t(Evt const &, S const &to, Guard &&p = nullptr, ActionT &&entry = nullptr, ActionT &&exit = nullptr) {
            std::string event_name{fsm_cxx::debug::type_name<Evt>()};
            Second s;
            s.emplace_back(StateT{to}, std::move(p), std::move(entry), std::move(exit));
            m_.emplace(std::move(First{event_name}), std::move(s));
        }
        template<typename Evt,
                 std::enable_if_t<std::is_same<Evt, Event>::value && !std::is_same<Evt, std::string>::value, bool> = true>
        transition_t(Evt const &, StateT const &to, Guard &&p = nullptr, ActionT &&entry = nullptr, ActionT &&exit = nullptr) {
            std::string event_name{fsm_cxx::debug::type_name<Evt>()};
            Second s;
            s.emplace_back(to, std::move(p), std::move(entry), std::move(exit));
            m_.emplace(std::move(First{event_name}), std::move(s));
        }
        transition_t(std::string const &event_name, StateT const &to, Guard &&p = nullptr, ActionT &&entry = nullptr, ActionT &&exit = nullptr) {
            Second s;
            s.emplace_back(to, std::move(p), std::move(entry), std::move(exit));
            m_.emplace(std::move(First{event_name}), std::move(s));
        }

        void add(transition_t &&t) {
            for (auto &[k, v] : t.m_) {
                if (auto it = m_.find(k); it == m_.end())
                    m_.emplace(k, v);
                else
                    for (auto &z : v)
                        it->second.emplace_back(std::move(z));
            }
        }

    public:
        std::tuple<bool, Item const &> get(std::string const &event_name, EventT const &ev, Context &ctx, Payload const &payload) const { return _get(event_name, ev, ctx, payload); }
        std::tuple<bool, Item &> get(std::string const &event_name, EventT const &ev, Context &ctx, Payload const &payload) { return _get(event_name, ev, ctx, payload); }
        std::tuple<bool, Item &> _get(std::string const &event_name, EventT const &ev, Context &ctx, Payload const &payload) {
            auto it = m_.find(event_name);
            if (it != m_.end()) {
                for (auto &v : it->second) {
                    if (v.verify(ev, ctx, payload))
                        return std::tuple<bool, Item &>{true, v};
                }
            }
            static Item s2{};
            return std::tuple<bool, Item &>{false, s2};
        }
    };

} // namespace fsm_cxx

// ----------------------------- machine_t
namespace fsm_cxx {

    AWESOME_MAKE_ENUM(Reason,
                      Unknown,
                      FailureGuard,
                      StateNotFound)

    template<typename S,
             typename EventT = event_t,
             typename MutexT = void, // or std::mutex
             typename PayloadT = payload_t,
             typename StateT = state_t<S>,
             typename ContextT = context_t<StateT, EventT, MutexT, PayloadT>,
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
        using Actions = detail::actions_t<S, Event, MutexT, Payload, State, Context, Action>;
        using Transition = transition_t<S, Event, MutexT, Payload, State, Context, Action>;
        using TransitionTable = std::unordered_map<State, Transition>;
        using OnAction = std::function<void(State const &, Event const &, State const &, typename Transition::Item const &, Payload const &)>;
        using OnErrorAction = std::function<void(Reason reason, State const &, Context &, Event const &, Payload const &)>;
        using StateActions = std::unordered_map<State, Actions>;
        using lock_guard_t = util::cool::lock_guard<MutexT>;
        using Guard = typename Transition::Guard;

    public:
        machine_t &reset() {
            _ctx.reset(_initial);
            return (*this);
        }
        
        machine_t &on_transition(OnAction &&fn) {
            _on_action = fn;
            return (*this);
        }
        machine_t &on_error(OnErrorAction &&fn) {
            _on_error = fn;
            return (*this);
        }

    protected:
        machine_t &initial_set(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            _initial = st;
            _ctx.current(st);
            return state_set(st, std::move(entry_action), std::move(exit_action));
        }
        machine_t &terminated_set(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            _terminated = st;
            return state_set(st, std::move(entry_action), std::move(exit_action));
        }
        machine_t &error_set(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            _error = st;
            return state_set(st, std::move(entry_action), std::move(exit_action));
        }

        machine_t &state_set(S st, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            Actions actions{std::move(entry_action), std::move(exit_action)};
            if (actions.valid())
                _state_actions.emplace(StateT{st}, std::move(actions));
            return (*this);
        }

        template<typename _Callable, typename... _Args>
        machine_t &guard_add(State const &st, _Callable &&f, _Args &&...args) {
            _ctx.add_guard(st, std::forward<_Callable>(f), std::forward<_Args>(args)...);
            return (*this);
        }

        template<typename Evt>
        machine_t &transition_set(S from, Evt const &, S to, Guard &&p = nullptr, ActionT &&entry_action = nullptr, ActionT &&exit_action = nullptr) {
            auto event_name = std::string{fsm_cxx::debug::type_name<Evt>()};
            Transition t{event_name, to, std::move(p), std::move(entry_action), std::move(exit_action)};
            return transition_set(from, std::move(t));
        }
        machine_t &transition_set(S from, Transition &&trans) {
            State f{from};
            return transition_set(f, std::forward<Transition>(trans));
        }
        machine_t &transition_set(State const &from, Transition &&trans) {
            if (auto it = _trans_tbl.find(from); it == _trans_tbl.end())
                _trans_tbl.emplace(from, std::move(trans));
            else
                it->second.add(std::forward<Transition>(trans));
            return (*this);
        }

    public:
        class state_builder {
            machine_t &owner;
            S st{};
            std::vector<Guard> guard_fn{};
            Action entry_fn{nullptr};
            Action exit_fn{nullptr};
            bool initial_, terminated_, error_;

        public:
            state_builder(machine_t &tt)
                : owner(tt) {}
            machine_t &build() {
                if (initial_) {
                    return owner.initial_set(st, std::move(entry_fn), std::move(exit_fn));
                } else if (terminated_) {
                    return owner.terminated_set(st, std::move(entry_fn), std::move(exit_fn));
                } else if (error_) {
                    return owner.error_set(st, std::move(entry_fn), std::move(exit_fn));
                }
                for (auto &fn : guard_fn)
                    owner.guard_add(st, fn);
                return owner.state_set(st, std::move(entry_fn), std::move(exit_fn));
            }
            state_builder &set(S s) {
                st = s;
                return (*this);
            }
            state_builder &as_initial() {
                initial_ = true;
                error_ = terminated_ = false;
                return (*this);
            }
            state_builder &as_terminated() {
                terminated_ = true;
                error_ = initial_ = false;
                return (*this);
            }
            state_builder &as_error() {
                error_ = true;
                initial_ = terminated_ = false;
                return (*this);
            }
            state_builder &guard(Guard &&fn) {
                guard_fn.emplace_back(fn);
                return (*this);
            }
            template<typename _Callable, typename... _Args>
            state_builder &entry_action(_Callable &&f, _Args &&...args) {
                entry_fn.update(f, args...);
                return (*this);
            }
            template<typename _Callable, typename... _Args>
            state_builder &exit_action(_Callable &&f, _Args &&...args) {
                exit_fn.update(f, args...);
                return (*this);
            }
        };
        state_builder state() { return state_builder(*this); }

    public:
        class transition_builder {
            machine_t &owner;
            S from{};
            std::string event_name{};
            S to{};
            Guard guard_fn{nullptr};
            Action entry_fn{nullptr};
            Action exit_fn{nullptr};

        public:
            transition_builder(machine_t &tt)
                : owner(tt) {}
            machine_t &build() { return owner.transition_set(from, Transition{event_name, to, std::move(guard_fn), std::move(entry_fn), std::move(exit_fn)}); }
            template<typename Evt,
                     std::enable_if_t<std::is_base_of<Event, std::decay_t<Evt>>::value && !std::is_same<Evt, std::string>::value, bool> = true>
            transition_builder &set(S from_, Evt const &, S to_) {
                from = from_;
                event_name = std::string{fsm_cxx::debug::type_name<Evt>()};
                to = to_;
                return (*this);
            }
            transition_builder &guard(Guard &&fn) {
                guard_fn = std::move(fn);
                return (*this);
            }
            template<typename _Callable, typename... _Args>
            transition_builder &entry_action(_Callable &&f, _Args &&...args) {
                entry_fn.update(f, args...);
                return (*this);
            }
            template<typename _Callable, typename... _Args>
            transition_builder &exit_action(_Callable &&f, _Args &&...args) {
                exit_fn.update(f, args...);
                return (*this);
            }
        };
        transition_builder transition() { return transition_builder(*this); }

    public:
        template<typename Evt,
                 std::enable_if_t<std::is_base_of<Event, std::decay_t<Evt>>::value && !std::is_same<Evt, std::string>::value, bool> = true>
        bool step_by(Evt const &ev) {
            std::string event_name{fsm_cxx::debug::type_name<Evt>()};
            return step_by(ev, Payload{});
        }
        template<typename Evt,
                 std::enable_if_t<std::is_base_of<Event, std::decay_t<Evt>>::value && !std::is_same<Evt, std::string>::value, bool> = true>
        bool step_by(Evt const &ev, Payload const &payload) {
            std::string event_name{fsm_cxx::debug::type_name<Evt>()};
            return step_by(event_name, ev, payload);
        }
        bool step_by(std::string const &event_name, Event const &ev, Payload const &payload) {
            auto reason = Reason::StateNotFound;

            typename TransitionTable::iterator it;
            lock_guard_t locker;
            auto &from = _ctx.current(); // reentrant is ok on the same lock/mutex.
            while ((it = _trans_tbl.find(from)) != _trans_tbl.end()) {
                auto &tr = it->second;
                auto [ok, item] = tr.get(event_name, ev, _ctx, payload);
                if (ok) {
                    auto &trans = item;

                    // verify state guards
                    if (!_ctx.verify(trans.to, ev, payload)) {
                        reason = Reason::FailureGuard;
                        break;
                    }

                    locker.unlock();

                    reason = Reason::Unknown;
                    auto leave = _state_actions.find(from);
                    trans.exit_action(ev, _ctx, from, payload);
                    if (leave != _state_actions.end())
                        leave->second.exit_action(ev, _ctx, trans.to, payload);

                    _ctx.current(trans.to);
                    if (_on_action)
                        _on_action(from, ev, trans.to, trans, payload);

                    auto enter = _state_actions.find(trans.to);
                    trans.entry_action(ev, _ctx, trans.to, payload);
                    if (enter != _state_actions.end())
                        enter->second.entry_action(ev, _ctx, from, payload);

                    // UNUSED(actions);
                    // fsm_debug("        [%s] -- %s --> [%s]", state_to_sting(_ctx.current).c_str(), event_name.c_str(), state_to_sting(to).c_str());
                    return true;
                }
                break;
            }
            if (_on_error)
                _on_error(reason, from, _ctx, ev, payload);
            return false;
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
        OnAction _on_action{}; // for debugging
        OnErrorAction _on_error{};
        StateActions _state_actions{}; // entry/exit actions for states
    };                                 // class machine_t

    template<typename S,
             typename EventT = event_t,
             typename PayloadT = payload_t>
    using safe_machine_t = machine_t<S, EventT, std::mutex, PayloadT>;

} // namespace fsm_cxx

#endif //FSM_CXX_FSM_SM_HH