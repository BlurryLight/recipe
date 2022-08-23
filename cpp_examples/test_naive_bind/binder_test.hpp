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
	  UnresolvedArgsList_{std::forward<TArgs>(args)...}
	{}
	
// Please C++, give me a way of detecting noexcept :'(
	template<class ... CallArgs>
	constexpr decltype(auto) operator()(CallArgs&&... args) 
	{
		return call(std::make_index_sequence<sizeof...(Args)>{}, std::forward<CallArgs>(args)...);
	}
	
private:
	template<class ... CallArgs, size_t ... Seq>
	constexpr decltype(auto) call(std::index_sequence<Seq...>, CallArgs&&... args) 
	{
		// 创建callee_List保存调用Operator()时候传入的参数,用于补齐占位符
		auto calleeList = callee_list<CallArgs...>{std::forward<CallArgs>(args)...};
		// 调用callee_list.operator[]

		// Redchards原版:
		// 参数折叠展开 f_(calleeList[argumentList_[index_constant<0>{}]],calleeList[argumentList_[index_constant<1>{}]],...);
		// return f_(calleeList[argumentList_[index_constant<Seq>{}]]...);

		return f_(calleeList[std::get<index_constant<Seq>{}>(UnresolvedArgsList_)]...);
	}
private:
	std::function<std::remove_reference_t<std::remove_pointer_t<Fn>>> f_;
	// 将占位符以及原有的参数保存在UnResolvedArgsLists_里，在call的时候填充占位符
	std::tuple<typename std::decay_t<Args>...> UnresolvedArgsList_;
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