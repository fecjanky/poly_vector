#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <malloc.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <variant>
#include <vector>

#include <poly/vector.h>

using namespace poly;

constexpr auto cache_size      = 8 * 1024 * 1024U;
constexpr auto cache_line_size = 64U;
constexpr auto page_size       = 4 * 1024;
constexpr auto align           = cache_line_size;
struct Interface {

    virtual void doYourThing() = 0;

    virtual std::string toString() const = 0;

    virtual ~Interface() = default;
};

class Implementation1 : public Interface {
public:
    Implementation1(int seed)
        : _current { seed }
    {
    }

    void doYourThing() override { _current += _current; }

    std::string toString() const override
    {
        return std::string("Implementation1(") + std::to_string(_current) + ")";
    }

private:
    int _current;
};

class Implementation2 : public Interface {
public:
    Implementation2(double op1, double)
        : _op1 { op1 }
    {
    }

    void doYourThing() override { _op1 = pow(_op1, _op1); }

    std::string toString() const override
    {
        return std::string("Implementation2(") + std::to_string(_op1) + ")";
    }

private:
    double _op1;
};

using namespace std::chrono;

template <typename T> T getArgv(int argc, char* argv[], int idx)
{
    if (argc <= idx)
        throw std::runtime_error("invalid num of arguments");
    std::istringstream iss(argv[idx]);
    iss.exceptions(std::istream::failbit | std::istream::badbit);
    T res {};
    iss >> res;
    return res;
}

template <typename T> struct TD;

namespace Measurements {
template <typename DT, typename F> auto timed(F func)
{
    return [=, f = std::move(func)](auto&&... args) {
        const auto start = std::chrono::high_resolution_clock::now();
        using r          = std::invoke_result_t<F, decltype(args)...>;
        if constexpr (std::is_same_v<r, void>) {
            std::invoke(std::move(func), std::forward<decltype(args)>(args)...);
            const auto end = std::chrono::high_resolution_clock::now();
            return std::make_pair(std::monostate {}, std::chrono::duration_cast<DT>(end - start));
        } else {
            auto       res = std::invoke(std::move(func), std::forward<decltype(args)>(args)...);
            const auto end = std::chrono::high_resolution_clock::now();
            return std::make_pair(std::move(res), std::chrono::duration_cast<DT>(end - start));
        }
    };
}

int f(int a) { return a; }

struct Benchmark {
    virtual std::chrono::microseconds run() = 0;
    virtual ~Benchmark()                    = default;
};

struct aligned_deleter {
    template <typename T> void operator()(T* ptr) const
    {
        using Type = std::remove_cv_t<T>;
        ptr->~Type();
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
};

template <typename T, typename... Args> T* aligned_new(Args&&... args)
{
#ifdef _WIN32
    auto p = static_cast<T*>(_aligned_malloc(page_size, align));
#else
    T* p {};
    posix_memalign(reinterpret_cast<void**>(&p), align, page_size);
#endif
    try {
        return new (p) T(std::forward<Args>(args)...);
    } catch (...) {
#ifdef _WIN32
        _aligned_free(p);
#else
        free(p);
#endif
    }
}

template <typename T> using a_unique_ptr = std::unique_ptr<T, aligned_deleter>;

template <typename Derived> struct BenchmarkBase : public Benchmark {
    bool         is_standard = false;
    unsigned int num_objs {}, iteration_count {};
    BenchmarkBase(int argc, char* argv[])
    {
        auto type       = getArgv<std::string>(argc, argv, 1);
        is_standard     = type == "std";
        num_objs        = getArgv<unsigned>(argc, argv, 2);
        iteration_count = getArgv<unsigned>(argc, argv, 3);
        std::cout << "Num of objs: " << num_objs << '\n';
    }
    Derived& derived() { return static_cast<Derived&>(*this); }
    auto     run_poly_vec()
    {
        for (auto c = 0U; c < iteration_count; c++) {
            std::for_each(
                derived().pv.begin(), derived().pv.end(), [](Interface& i) { i.doYourThing(); });
        }
    }

    auto run_std_vec()
    {
        for (auto c = 0U; c < iteration_count; c++) {
            std::for_each(
                derived().sv.begin(), derived().sv.end(), [](auto& i) { i->doYourThing(); });
        }
    }
};

struct WorstCase : public BenchmarkBase<WorstCase> {
    vector<Interface>                    pv;
    std::vector<a_unique_ptr<Interface>> sv;

    WorstCase(int argc, char* argv[])
        : BenchmarkBase(argc, argv)
    {
        constexpr auto size = std::max(sizeof(Implementation1), sizeof(Implementation2));

        if (is_standard) {
            sv.reserve(num_objs);
            for (auto i = 0U; i < num_objs; ++i) {
                static_assert(page_size > size, "this should not happen...");
                a_unique_ptr<Interface> p;
                if (std::rand() % 2) {
                    p.reset(aligned_new<Implementation1>(std::rand()));
                } else {
                    p.reset(aligned_new<Implementation2>(1.1, 1.3));
                }
                sv.push_back(std::move(p));
            }
            // Shuffle pointers for random allocation simulation
            std::next_permutation(sv.begin(), sv.end());
        } else {
            pv.reserve(num_objs, size, align);
            for (auto i = 0U; i < num_objs; ++i) {
                static_assert(page_size > size, "this should not happen...");
                if (std::rand() % 2) {
                    pv.push_back(Implementation1(std::rand()));
                } else {
                    pv.push_back(Implementation2(1.1, 1.3));
                }
            }
        }
    }
    std::chrono::microseconds run() override
    {
        if (is_standard) {
            auto res = timed<std::chrono::microseconds>(&BenchmarkBase::run_std_vec)(*this);
            std::cout << "vector:" << res.second.count() << " us\n";
            return res.second;
        } else {
            auto res = timed<std::chrono::microseconds>(&BenchmarkBase::run_poly_vec)(*this);
            std::cout << "poly_vec:" << res.second.count() << " us\n";
            return res.second;
        }
    }
};

struct BestCase : public BenchmarkBase<BestCase> {
    vector<Interface>                       pv;
    std::vector<std::unique_ptr<Interface>> sv;

    BestCase(int argc, char* argv[])
        : BenchmarkBase(argc, argv)
    {
        constexpr auto size = std::max(sizeof(Implementation1), sizeof(Implementation2));

        if (is_standard) {
            for (auto i = 0U; i < num_objs; ++i) {
                if (std::rand() % 2) {
                    sv.emplace_back(std::make_unique<Implementation1>(std::rand()));
                } else {
                    sv.emplace_back(std::make_unique<Implementation2>(1.1, 1.3));
                }
            }
        } else {
            for (auto i = 0U; i < num_objs; ++i) {
                static_assert(page_size > size, "this should not happen...");
                if (std::rand() % 2) {
                    pv.emplace_back<Implementation1>(std::rand());
                } else {
                    pv.emplace_back<Implementation2>(1.1, 1.3);
                }
            }
        }
    }
    std::chrono::microseconds run() override
    {
        if (is_standard) {
            auto res = timed<std::chrono::microseconds>(&BenchmarkBase::run_std_vec)(*this);
            std::cout << "vector: " << res.second.count() << " us\n";
            return res.second;
        } else {
            auto res = timed<std::chrono::microseconds>(&BenchmarkBase::run_poly_vec)(*this);
            std::cout << "poly_vec: " << res.second.count() << " us\n";
            return res.second;
        }
    }
};

struct CountingAllocatorBase {

    void count_alloc(size_t size_)
    {
        alloc_count++;
        size += size_;
    }
    void count_dealloc() { dealloc_count++; }

    static std::atomic<size_t> alloc_count;
    static std::atomic<size_t> dealloc_count;
    static std::atomic<size_t> size;
};

template <typename T> struct CountingAllocator : CountingAllocatorBase {
    using value_type                            = T;
    using is_always_equal                       = std::true_type;
    CountingAllocator()                         = default;
    CountingAllocator(const CountingAllocator&) = default;
    CountingAllocator(CountingAllocator&&)      = default;
    CountingAllocator& operator=(CountingAllocator&&) = default;
    CountingAllocator& operator=(const CountingAllocator&) = default;

    template <typename U> CountingAllocator(CountingAllocator<U>) { }

    T* allocate(size_t n)
    {
        const auto size = sizeof(T) * n;
        count_alloc(size);
        return static_cast<T*>(operator new(size));
    }
    void deallocate(T* p, size_t n)
    {
        count_dealloc();
        operator delete(p);
    }

    bool operator==(const CountingAllocator& rhs) const { return true; }
    bool operator!=(const CountingAllocator& rhs) const { return false; }
};

std::atomic<size_t> CountingAllocatorBase::alloc_count   = 0;
std::atomic<size_t> CountingAllocatorBase::dealloc_count = 0;
std::atomic<size_t> CountingAllocatorBase::size          = 0;

template <typename T> struct CountingDeleter {
    CountingDeleter() = default;
    template <typename U> CountingDeleter(CountingDeleter<U>) { }

    void operator()(T* p) const
    {
        CountingAllocator<T> a;
        using traits = std::allocator_traits<CountingAllocator<T>>;
        traits::destroy(a, p);
        traits::deallocate(a, p, 1);
    }
};

template <typename T> using c_unique_ptr = std::unique_ptr<T, CountingDeleter<T>>;

template <typename T, typename... Args> c_unique_ptr<T> make_unique_counted(Args&&... args)
{
    CountingAllocator<T> a;
    using traits = std::allocator_traits<CountingAllocator<T>>;
    auto p       = traits::allocate(a, 1);
    try {
        traits::construct(a, p, std::forward<Args>(args)...);
        return c_unique_ptr<T>(p);
    } catch (...) {
        traits::deallocate(a, p, 1);
        throw;
    }
}

struct AllocCount : public BenchmarkBase<AllocCount> {
    vector<Interface, CountingAllocator<Interface>>                                  pv;
    std::vector<c_unique_ptr<Interface>, CountingAllocator<c_unique_ptr<Interface>>> sv;

    AllocCount(int argc, char* argv[])
        : BenchmarkBase(argc, argv)
    {
        for (auto i = 0U; i < num_objs; ++i) {
            if (is_standard) {
                if (std::rand() % 2) {
                    sv.push_back(make_unique_counted<Implementation1>(std::rand()));
                } else {
                    sv.push_back(make_unique_counted<Implementation2>(1.1, 1.3));
                }
            } else {
                if (std::rand() % 2) {
                    pv.push_back(Implementation1(std::rand()));
                } else {
                    pv.push_back(Implementation2(1.1, 1.3));
                }
            }
        }
    }
    std::chrono::microseconds run() override
    {
        auto res = is_standard
            ? timed<std::chrono::microseconds>(&BenchmarkBase::run_std_vec)(*this)
            : timed<std::chrono::microseconds>(&BenchmarkBase::run_poly_vec)(*this);

        std::cout << (is_standard ? "vector" : "poly_vector")
                  << ": alloc count=" << CountingAllocatorBase::alloc_count
                  << ", total size=" << CountingAllocatorBase::size << '\n';
        return res.second;
    }
};

std::unique_ptr<Benchmark> get_measurement(const std::string_view& name, int argc, char* argv[])
{
    if (name == "WorstCase")
        return std::make_unique<WorstCase>(argc, argv);
    else if (name == "AllocCount")
        return std::make_unique<AllocCount>(argc, argv);
    else if (name == "BestCase")
        return std::make_unique<BestCase>(argc, argv);
    throw std::runtime_error(std::string("Invalid name:") + std::string(name));
}

}

int main(int argc, char* argv[])
try {
    auto measurement
        = Measurements::get_measurement(getArgv<std::string>(argc, argv, 4), argc, argv);
    auto res = measurement->run();
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    const char* help = "%s <obj count> <iteration count>";
    std::printf(help, argv[0]);
    return 1;
} catch (...) {
    std::cerr << "Unknown exception\n";
    return 1;
}
