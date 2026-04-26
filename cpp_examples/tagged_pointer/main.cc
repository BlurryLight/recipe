#include <cassert>
#include <cstddef>
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
  private:
    static constexpr int tagShift = 57;
    static constexpr int tagBits = 64 - tagShift;
    static constexpr uint64_t tagMask = ((uint64_t{1} << tagBits) - 1) << tagShift;
    static constexpr uint64_t ptrMask = ~tagMask;

  public:
    static_assert(sizeof(uintptr_t) <= sizeof(uint64_t));
    static_assert(sizeof...(Ts) < (1u << tagBits), "Too many tagged pointer types");

    TaggedPointer() = default;
    TaggedPointer(std::nullptr_t) {}

    template <typename T>
    explicit TaggedPointer(T *ptr) {
        set(ptr);
    }

    template <typename T>
    void set(T *ptr) {
        const auto iptr = reinterpret_cast<uint64_t>(ptr);
        assert((iptr & ptrMask) == iptr && "Pointer uses bits reserved for tag");

        const auto typeTag = static_cast<uint64_t>(type_index<T>());
        bits_ = iptr | (typeTag << tagShift);
    }

    template <typename T>
    static constexpr int type_index() {
        return 1 + TypeIndex<std::remove_cv_t<T>, Ts...>();
    }

    static constexpr int max_tag() { return sizeof...(Ts); }
    static constexpr int num_tags() { return max_tag() + 1; }

    int tag() const { return static_cast<int>((bits_ & tagMask) >> tagShift); }
    void *ptr() { return reinterpret_cast<void *>(bits_ & ptrMask); }
    const void *ptr() const { return reinterpret_cast<const void *>(bits_ & ptrMask); }

    explicit operator bool() const { return ptr() != nullptr; }

    template <typename T>
    bool is() const {
        return tag() == type_index<T>();
    }

    template <typename T>
    T *cast() {
        assert(is<T>());
        return static_cast<T *>(ptr());
    }

    template <typename T>
    const T *cast() const {
        assert(is<T>());
        return static_cast<const T *>(ptr());
    }

    template <typename T>
    T *cast_or_nullptr() {
        return is<T>() ? static_cast<T *>(ptr()) : nullptr;
    }

    template <typename T>
    const T *cast_or_nullptr() const {
        return is<T>() ? static_cast<const T *>(ptr()) : nullptr;
    }

    template <typename F>
    decltype(auto) dispatch(F &&func) {
        assert(ptr() != nullptr);
        return DispatchCPU<F, Ts...>(std::forward<F>(func), ptr(), tag() - 1);
    }

    template <typename F>
    decltype(auto) dispatch(F &&func) const {
        assert(ptr() != nullptr);
        return DispatchCPU<F, Ts...>(std::forward<F>(func), ptr(), tag() - 1);
    }

  private:
    uint64_t bits_ = 0;
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

        assert(ShapePtr::max_tag() == 3);
        assert(ShapePtr::num_tags() == 4);
        assert(ShapePtr::type_index<Sphere>() == 1);
        assert(ShapePtr::type_index<Triangle>() == 2);
        assert(ShapePtr::type_index<Curve>() == 3);

        ShapePtr empty;
        assert(!empty);
        assert(empty.ptr() == nullptr);
        assert(empty.tag() == 0);

        ShapePtr nullShape(nullptr);
        assert(!nullShape);
        assert(nullShape.ptr() == nullptr);
        assert(nullShape.tag() == 0);

        ShapePtr shape(&triangle);
        assert(shape);
        assert(shape.ptr() == &triangle);
        assert(shape.tag() == ShapePtr::type_index<Triangle>());
        assert(shape.is<Triangle>());
        assert(!shape.is<Sphere>());
        assert(shape.cast<Triangle>() == &triangle);
        assert(shape.cast_or_nullptr<Triangle>() == &triangle);
        assert(shape.cast_or_nullptr<Sphere>() == nullptr);

        auto id = shape.dispatch([](auto *p) { return p->id(); });
        assert(id == 2);

        const ShapePtr constShape(&curve);
        assert(constShape);
        assert(constShape.ptr() == &curve);
        assert(constShape.cast<Curve>() == &curve);
        assert(constShape.cast_or_nullptr<Curve>() == &curve);
        assert(constShape.cast_or_nullptr<Sphere>() == nullptr);
        auto constId = constShape.dispatch([](const auto *p) { return p->id(); });
        assert(constId == 3);

        int rawVoidSum = 0;
        shape.dispatch([&](auto *p) { rawVoidSum += p->id(); });
        assert(rawVoidSum == 2);

        constShape.dispatch([&](const auto *p) { rawVoidSum += p->id(); });
        assert(rawVoidSum == 5);

        shape.set(&sphere);
        assert(shape.ptr() == &sphere);
        assert(shape.tag() == ShapePtr::type_index<Sphere>());
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
