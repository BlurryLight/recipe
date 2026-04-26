#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <utility>

template <typename>
inline constexpr bool always_false_v = false;

template <typename T, typename First, typename... Rest>
constexpr int TypeIndex() {
    if constexpr (std::is_same_v<T, First>) {
        return 0;
    } else if constexpr (sizeof...(Rest) > 0) {
        return 1 + TypeIndex<T, Rest...>();
    } else {
        static_assert(always_false_v<T>, "Type is not in TaggedPointer type list");
    }
}

template <typename F, typename T, typename... Rest>
decltype(auto) DispatchCPU(F &&func, void *ptr, int index) {
    if (index == 0)
        return std::forward<F>(func)(static_cast<T *>(ptr));

    if constexpr (sizeof...(Rest) > 0) {
        return DispatchCPU<F, Rest...>(std::forward<F>(func), ptr, index - 1);
    } else {
        assert(false && "Invalid tag index");
        return std::forward<F>(func)(static_cast<T *>(ptr));
    }
}

template <typename F, typename T, typename... Rest>
decltype(auto) DispatchCPU(F &&func, const void *ptr, int index) {
    if (index == 0)
        return std::forward<F>(func)(static_cast<const T *>(ptr));

    if constexpr (sizeof...(Rest) > 0) {
        return DispatchCPU<F, Rest...>(std::forward<F>(func), ptr, index - 1);
    } else {
        assert(false && "Invalid tag index");
        return std::forward<F>(func)(static_cast<const T *>(ptr));
    }
}

template <typename... Ts>
class TaggedPointer {
  public:
    TaggedPointer() = default;

    template <typename T>
    explicit TaggedPointer(T *ptr) {
        set(ptr);
    }

    template <typename T>
    void set(T *ptr) {
        ptr_ = ptr;
        tag_ = TypeIndex<std::remove_cv_t<T>, Ts...>();
    }

    int tag() const { return tag_; }
    void *ptr() { return ptr_; }
    const void *ptr() const { return ptr_; }

    template <typename T>
    bool is() const {
        return tag_ == TypeIndex<std::remove_cv_t<T>, Ts...>();
    }

    template <typename F>
    decltype(auto) dispatch(F &&func) {
        assert(ptr_ != nullptr);
        return DispatchCPU<F, Ts...>(std::forward<F>(func), ptr_, tag_);
    }

    template <typename F>
    decltype(auto) dispatch(F &&func) const {
        assert(ptr_ != nullptr);
        return DispatchCPU<F, Ts...>(std::forward<F>(func), ptr_, tag_);
    }

  private:
    void *ptr_ = nullptr;
    int tag_ = -1;
};

struct Sphere {
    int id() const { return 1; }
    void visit(int &log) { log = log * 10 + id(); }
    void inspect(int &log) const { log = log * 10 + id(); }
};

struct Triangle {
    int id() const { return 2; }
    void visit(int &log) { log = log * 10 + id(); }
    void inspect(int &log) const { log = log * 10 + id(); }
};

struct Curve {
    int id() const { return 3; }
    void visit(int &log) { log = log * 10 + id(); }
    void inspect(int &log) const { log = log * 10 + id(); }
};


struct PrimitiveHandle : public TaggedPointer<Sphere, Triangle, Curve>
{
    using TaggedPointer::TaggedPointer; // inherit explicit constructor
    int id() const
    {
        auto f = [&](auto ptr) { return ptr->id();};
        return dispatch(f);
    }

    void add_id_to(int &sum) const
    {
        auto f = [&](auto ptr) { sum += ptr->id(); };
        dispatch(f);
    }

    void visit(int &log)
    {
        return dispatch([&](auto *ptr) { ptr->visit(log); });
    }

    void inspect(int &log) const
    {
        return dispatch([&](const auto *ptr) { ptr->inspect(log); });
    }
};

static_assert(std::is_void_v<decltype(std::declval<PrimitiveHandle &>().visit(
    std::declval<int &>()))>);
static_assert(std::is_void_v<decltype(std::declval<const PrimitiveHandle &>().inspect(
    std::declval<int &>()))>);


int main() {
    {
        // region: Raw Test
        using ShapePtr = TaggedPointer<Sphere, Triangle, Curve>;

        Sphere sphere;
        Triangle triangle;
        Curve curve;

        ShapePtr shape(&triangle);
        assert(shape.is<Triangle>());
        assert(!shape.is<Sphere>());

        auto id = shape.dispatch([](auto *p) { return p->id(); });
        assert(id == 2);

        const ShapePtr constShape(&curve);
        auto constId = constShape.dispatch([](const auto *p) { return p->id(); });
        assert(constId == 3);

        int rawVoidSum = 0;
        shape.dispatch([&](auto *p) { rawVoidSum += p->id(); });
        assert(rawVoidSum == 2);

        constShape.dispatch([&](const auto *p) { rawVoidSum += p->id(); });
        assert(rawVoidSum == 5);

        shape.set(&sphere);
        assert(shape.dispatch([](auto *p) { return p->id(); }) == 1);
    }
    // region: Raw End
    
    // region: Handle Test
    {
        Sphere sphere;
        PrimitiveHandle handle(&sphere);
        assert(sphere.id() == 1);
        assert(handle.id() == 1);

        Triangle tri;
        handle.set(&tri);
        assert(handle.id() == 2);

        int handleVoidSum = 0;
        handle.add_id_to(handleVoidSum);
        assert(handleVoidSum == 2);

        int polymorphicVoidLog = 0;
        handle.visit(polymorphicVoidLog);
        assert(polymorphicVoidLog == 2);

        const PrimitiveHandle constHandle(&sphere);
        constHandle.inspect(polymorphicVoidLog);
        assert(polymorphicVoidLog == 21);

    }
    // region: Handle Test End

    std::cout << "tagged pointer example passed\n";
    return 0;
}
