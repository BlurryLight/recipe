#pragma once
#include "binderlist_test.hpp"
#include "calleelist_test.hpp"
template<class Fn, class ... Args>
class binder
{
public:
	template<class TFn, class ... TArgs>
	constexpr binder(TFn&& f, TArgs&&... args) noexcept 
	: f_{std::forward<TFn>(f)},
	  argumentList_{std::forward<TArgs>(args)...}
	{}
	
// Please C++, give me a way of detecting noexcept :'(
	template<class ... CallArgs>
	constexpr decltype(auto) operator()(CallArgs&&... args) 
	//noexcept(noexcept(call(std::make_index_sequence<sizeof...(Args)>{}, std::declval<Args>()...)))
	{
		return call(std::make_index_sequence<sizeof...(Args)>{}, std::forward<CallArgs>(args)...);
	}
	
private:
	template<class ... CallArgs, size_t ... Seq>
	constexpr decltype(auto) call(std::index_sequence<Seq...>, CallArgs&&... args) 
	//noexcept(noexcept(f_(this->binder_list<CallArgs...>{std::declval<CallArgs>()...}[this->argumentList_[index_constant<Seq>{}]]...)))
	{
		return f_((callee_list<CallArgs...>{std::forward<CallArgs>(args)...}[argumentList_[index_constant<Seq>{}]])...);
	}
private:
	std::function<std::remove_reference_t<std::remove_pointer_t<Fn>>> f_;
	binder_list<Args...> argumentList_;
};

template<class Fn, class ... Args>
// The code is from RedChards https://gist.github.com/Redchards/c5be14c2998f1ca1d757
binder<Fn, Args...> Redchards_Bind(Fn&& f, Args&&... args)
{
	return binder<Fn, Args...>{std::forward<Fn>(f), std::forward<Args>(args)...};
}


int foo(int a,int& b,std::string c)
{
    std::cout << a << std::endl;
    std::cout << c << std::endl;
    return ++b;
}
int test_binder()
{
    int n = 1;
    auto fn = Redchards_Bind(foo, 100,std::ref(n),std::placeholders::_1);
    int ret_val = fn("hello bind");
    assert(ret_val == 2);
    assert(n == 2);

    ret_val = fn("hello bind");
    assert(ret_val == 3);
    assert(n == 3);
    return 0;
}