
#pragma once
#include <functional>
#include "typename.h"
#include <iostream>
#include <cassert>

template<class ... Args>
class callee_list
{
public:
	template<class ... TArgs>
	constexpr callee_list(TArgs&&... args) noexcept
	: boundedArgs_{std::forward<TArgs>(args)...}
	{}
	

  // Original SFINAE
	// template<class T, std::enable_if_t<(std::is_placeholder<std::remove_reference_t<T>>::value == 0)>* = nullptr>
  // constexpr decltype(auto) operator[](T&& t) noexcept
	// {
  //       return std::forward<T>(t);
	// }
	
	// template<class T, std::enable_if_t<(std::is_placeholder<T>::value != 0)>* = nullptr>
	// constexpr decltype(auto) operator[](T) noexcept
	// {
	// 	return std::get<std::is_placeholder<T>::value - 1>(std::move(boundedArgs_));
	// }

	template<class T>
  constexpr decltype(auto) operator[](T&& t) noexcept
	{
    if constexpr (!std::is_placeholder_v<std::decay_t<T>>)
    {
        return std::forward<T>(t);
    }
    else
    {
      constexpr size_t Index = std::is_placeholder<std::decay_t<T>>::value - 1;
      return std::get<Index>(std::move(boundedArgs_));
    }
	}

  // Bind以值类型存储所有变量，会擦除int& 到int
	std::tuple<typename std::decay_t<Args>...> boundedArgs_;	
};

template <class... Args> decltype(auto) make_callee_list(Args &&...args) {
  return callee_list<Args...>{std::forward<Args>(args)...};
}

inline void test_callee_list()
{
    auto lst = make_callee_list('a',2.0,3);
    std::cout << type_name<decltype(lst.boundedArgs_)>()<<std::endl;

    // 返回对应的值
    assert(lst[5] == 5);
    char a = 'b';
    const char& a_ref = a;
    assert(lst[a_ref] == 'b');

    // 传入占位符，返回占位符序号对应的值
    assert(lst[std::placeholders::_1] == 'a');
    assert(lst[std::placeholders::_2] == 2.0);
    assert(lst[std::placeholders::_3] == 3);
}