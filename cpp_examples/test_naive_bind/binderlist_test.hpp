#pragma once
#include "typename.h"
#include <functional>
#include <iostream>
#include <tuple>
#include <cassert>

template <size_t n> using index_constant = std::integral_constant<size_t, n>;

template <class... Args> class binder_list {
public:
  template <class... TArgs>
  constexpr binder_list(TArgs &&...args) noexcept
      : boundedArgs_{std::forward<TArgs>(args)...} {}

  template <size_t n>
  constexpr auto operator[](index_constant<n>) noexcept 
  {
    return std::get<n>(boundedArgs_);
  }

private:
  std::tuple<typename std::decay_t<Args>...> boundedArgs_;
};

template <class... Args> decltype(auto) make_binder_list(Args &&...args) {
  return binder_list<Args...>{std::forward<Args>(args)...};
}

inline void test_binder_list() {
  using namespace std::placeholders;
  // binder_list lst(1,2,3); // will be error, cannot deduce type from args list
  int a = 3;
  auto lst = make_binder_list(1, 2, _1,std::ref(a));
  assert(lst[index_constant<0>()] == 1);
  assert(lst[index_constant<1>()] == 2);

  decltype(lst[index_constant<2>()]) b;
  static_assert(std::is_same<decltype(b), std::remove_cv_t<decltype(std::placeholders::_1)>>::value);
  static_assert(std::is_placeholder<std::remove_reference<
                   decltype(lst[index_constant<2>()])>::type>::value > 0 );

  // 验证 std::ref(int) 往 int&  的隐式转换
  // static_assert(std::is_convertible<int&, decltype(std::declval<decltype(lst[index_constant<3>()])>().operator int& ())>::value);
  static_assert(std::is_convertible<int&, decltype(lst[index_constant<3>()])>::value);
  std::cout << type_name<decltype(lst[index_constant<2>()])>() << std::endl;
}