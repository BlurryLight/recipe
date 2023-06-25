
#include <iostream>
#include <memory>   // std::allocator
#include <vector>
#include <memory_resource>
#include <array>

// extented version
// inspired by https://stackoverflow.com/a/72771645

// template<class T,size_t N>
// struct limited_allocator {
//     using value_type = typename std::allocator<T>::value_type;
//     using size_type = typename std::allocator<T>::size_type;
//     using difference_type = std::ptrdiff_t;
//     using propagate_on_container_move_assignment = std::true_type;

//     using is_always_equal = std::true_type; // not needed since C++23
    
//     // No default constructor - it needs a limit:

//     limited_allocator() = default;
//     constexpr limited_allocator( const limited_allocator& other ) noexcept = default;

//     template <class U, size_t M>
//     constexpr limited_allocator(const limited_allocator<U, M> &) noexcept{}
//     // Implementing this is what enforces the limit:
//     size_type max_size() const noexcept { return N; }

//     template<class Other>
//     struct rebind {
//         typedef limited_allocator<Other,N> other;
//     };

 
//     [[nodiscard]] T* allocate(std::size_t n)
//     {
//         return alloc.allocate(n);
//     }
 
//     void deallocate(T* p, std::size_t n) noexcept
//     {
//         alloc.deallocate(p,n);
//     }
// private:
//     std::allocator<T> alloc;
// };

// compact version
// https://en.cppreference.com/w/cpp/named_req/Allocator
// https://stackoverflow.com/questions/48061522/create-the-simplest-allocator-with-two-template-arguments
// check allocator completeness here 
template <class T, size_t N> struct limited_allocator : std::allocator<T> {
  using value_type = typename std::allocator<T>::value_type;
  using size_type = typename std::allocator<T>::size_type;
  size_type max_size() const noexcept { return N; }
  template <class Other> struct rebind {
    typedef limited_allocator<Other, N> other;
  };

  // make msvc happy
  template <class Other,size_t M> 
  constexpr operator limited_allocator<Other, M>() const noexcept { return limited_allocator<Other, M>(); }
};

template <class T, size_t N> struct limited_pmr_allocator : std::pmr::polymorphic_allocator<T> {
  using value_type = typename std::pmr::polymorphic_allocator<T>::value_type;
  using size_type = typename std::allocator_traits<std::pmr::polymorphic_allocator<T>>::size_type;
  size_type max_size() const noexcept { return N; }
  template <class Other> struct rebind {
    typedef limited_pmr_allocator<Other, N> other;
  };

  template <class Other,size_t M> 
  constexpr operator limited_pmr_allocator<Other, M>() const noexcept { return limited_pmr_allocator<Other, M>{this->resource()}; }
};

int main() {
  auto CheckVec = [](auto &vec, auto &al) {
    try {
      vec.reserve(4);
      vec.push_back(1); // one element
      vec.push_back(1); // one element
      vec.push_back(1); // one element
      vec.push_back(1); // one element
      vec.push_back(1); // one element
      vec.push_back(1); // one element
      for (int i = 0; i < 50; i++) {
        vec.push_back(i);
      }
    } catch (const std::length_error &ex) {
      std::cout << "length_error: " << ex.what() << '\n';
    } catch (const std::bad_array_new_length &ex) {
      std::cout << "bad_array_new_length: " << ex.what() << '\n';
    } catch (const std::bad_alloc &ex) {
      std::cout << "bad_alloc: " << ex.what() << '\n';
    }
    std::cout << vec.size() << std::endl;
    std::cout << std::allocator_traits<std::remove_reference_t<decltype(al)>>::max_size(al) << " "
              << vec.max_size() << std::endl;
  };

  {
    auto al = limited_allocator<int, 10>();
    std::vector<int, limited_allocator<int, 10>> vec{};
    CheckVec(vec, al);
  }

  {

    std::array<char, 1024 * 100> buffer{};
    std::pmr::monotonic_buffer_resource resource(buffer.data(), buffer.size());
    auto al = limited_pmr_allocator<int, 10>{&resource};
    std::vector<int, limited_pmr_allocator<int, 10>> vec{al};
    CheckVec(vec, al);
  }
}