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
    rect(double t, double l, double b, double r) : _t(t), _l(l), _b(b), _r(r) {}
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

  struct canvas {
    using elemsp = std::shared_ptr<elem>;
    using elements = std::vector<elemsp>;
    elements _coll;

    template<typename... Args>
    elemsp create(Args &&...args) {
      auto sp = _coll.emplace_back(std::forward<Args>(args)...);
      return sp;
    }
  };

} // namespace a1

int main() {
  using namespace a1;
  canvas c;
  auto rc = c.create(1, 2, 3, 4);
  std::cout << rc->width() << '\n';
}