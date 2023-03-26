#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "fsm_cxx/fsm-common.hh"
#include "fsm_cxx/fsm-def.hh"

namespace a1 {

  class elem : public std::enable_shared_from_this<elem> {
  public:
    elem() = default;
    virtual ~elem() = default;
    auto self() { return shared_from_this(); }
    auto self() const { return shared_from_this(); }
  };

  template<typename P>
  class elem_t : public elem {
  public:
    elem_t() = default;
    ~elem_t() override = default;
    // using sp = std::shared_ptr<elem<P>>;

    P &This() { return static_cast<P &>(*this); }
    P const &This() const { return static_cast<P const &>(*this); }
  };

  class rect : public elem_t<rect> {
  public:
    rect() = default;
    ~rect() override = default;
    template<class T>
    rect(T const &t, T const &l, T const &b, T const &r) : _t(t), _l(l), _b(b), _r(r) {}
    rect(rect &&o) : _t(std::move(o._t)), _l(std::move(o._l)), _b(std::move(o._b)), _r(std::move(o._r)) {}
#ifndef __COPY
#define __COPY(m) this->m = std::move(o.m)
#endif
    rect &operator=(rect &&o) {
      __COPY(_t);
      __COPY(_l);
      __COPY(_b);
      __COPY(_r);
      return (*this);
    }

    auto width() const { return _r - _l; }
    auto height() const { return _b - _t; }

  private:
    double _t, _l, _b, _r;
  };
  struct generic_elem : public std::variant<std::monostate, rect> {
    generic_elem() = default;
    ~generic_elem() = default;
    
    using base_type = std::variant<std::monostate, rect>;
    using base_type::base_type;

    template<typename... Args>
    generic_elem(Args &&...args) : base_type(std::forward<Args>(args)...) {}
  };

  struct canvas {
    using elemsp = std::shared_ptr<generic_elem>;
    using elements = std::vector<generic_elem>;
    elements _coll;

    template<typename... Args>
    auto &create(Args &&...args) {
      auto &sp = _coll.emplace_back(std::forward<Args>(args)...);
      return sp;
    }
  };

} // namespace a1

int main() {
  using namespace a1;

  generic_elem el{std::in_place_type<rect>, 1, 2, 3, 4};
  std::cout << std::get<rect>(el).width() << '\n';

  generic_elem el2{std::in_place_index<1>, 1., 2., 3., 4.};
  std::cout << std::get<rect>(el2).width() << '\n';

  canvas c;
  auto &rc = c.create(std::in_place_index<1>, 1, 2, 3, 4);
  std::cout << std::get<a1::rect>(rc).width() << '\n';
}