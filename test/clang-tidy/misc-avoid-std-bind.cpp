// RUN: %check_clang_tidy %s misc-avoid-std-bind %t
//#include <functional>

namespace std {
template <class Fp, class... Arguments>
class bind_rt {};

template <class Fp, class... Arguments>
bind_rt<Fp, Arguments...> bind(Fp&&, Arguments&& ...);
}

int add(int x, int y) { return x + y; }

void f()
{
  auto clj = std::bind(add,2,2);
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: use of std::bind is deprecated [misc-avoid-std-bind]
}

// CHECK-FIXES: auto clj = [] { return add(2, 2); };

struct Adder
{
  int operator()() const { return x_ + y_; }

  Adder(int x, int y) : x_(x), y_(y) {}

  int x_ = 0;
  int y_ = 0;
};

void f2()
{
  auto clj = Adder(2,2);
}

