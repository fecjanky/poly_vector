#include <vector>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <malloc.h>
#include <stdlib.h>
#include <algorithm>
#include <chrono>

#include <poly_vector.h>


using namespace estd;

constexpr auto cache_size       = 8 * 1024 * 1024U;
constexpr auto cache_line_size  = 64U;
constexpr auto page_size        = 4 * 1024;

struct Interface {

    virtual void doYourThing() = 0;

    virtual std::string toString() const = 0;

    virtual ~Interface() = default;
};


class Implementation1 : public Interface {
public:
    Implementation1(int seed)// : _current{ seed } 
    {}

    void doYourThing() override
    {
        //_current += _current;
    }

    std::string toString() const override
    {
        return std::string("Implementation1(");// + std::to_string(_current) + ")";
    }
private:
    //int _current;
};

class Implementation2 : public Interface {
public:

    Implementation2(double op1, double) //: _op1{ op1 }
    {}

    void doYourThing() override
    {
        //_op1 = pow(_op1, _op1);
    }

    std::string toString() const override
    {
        return std::string("Implementation2(");//) + std::to_string(_op1) + ")";
    }
private:
    //double _op1;
};

constexpr auto num_objs = 2 * cache_size / std::max(sizeof(Implementation1),sizeof(Implementation2));

using namespace std::chrono;

int main(int argc, char*argv[]) try {

    unsigned int iteration_count{};
    if (argc < 2)throw std::runtime_error("invalid num of arguments");
    {
        std::istringstream iss(argv[1]);
        iss.exceptions(std::istream::failbit | std::istream::badbit);
        iss >> iteration_count;
    }
    constexpr auto align = cache_line_size;
    constexpr auto size = std::max(sizeof(Implementation1), sizeof(Implementation2));
    poly_vector<Interface> pv;
    std::vector<Interface*> sv;
    pv.reserve(num_objs, size, align);
    sv.reserve(num_objs);
    for (auto i = 0; i < num_objs; ++i) {
        static_assert(page_size > size, "this should not happen...");
        #ifdef _WIN32
        auto p = static_cast<Interface*>(_aligned_malloc(page_size, align));
        #else
        Interface* p{};
        posix_memalign(reinterpret_cast<void**>(&p), align, page_size);
        #endif
        if (std::rand() % 2) {
            pv.push_back(Implementation1(std::rand()));
            p = new(p) Implementation1(std::rand());
        }
        else {
            pv.push_back(Implementation2(1.1, 1.3));
            p = new(p) Implementation2(1.1, 1.3);
        }
        sv.push_back(p);
    }

    // Shuffle pointers for random allocation simulation
    std::random_shuffle(sv.begin(), sv.end());


    auto start_seq = std::chrono::high_resolution_clock::now();
    for (auto c = 0U; c < iteration_count; c++) {
        std::for_each(pv.begin(), pv.end(), [](Interface& i) { i.doYourThing(); });
    }
    auto end_seq = std::chrono::high_resolution_clock::now();

    auto start_rand = std::chrono::high_resolution_clock::now();
    for (auto c = 0U; c < iteration_count; c++) {
        std::for_each(sv.begin(), sv.end(), [](Interface* i) {i->doYourThing(); });
    }
    auto end_rand = std::chrono::high_resolution_clock::now();

    std::cout << "poly_vec:" << std::chrono::duration_cast<milliseconds>(end_seq - start_seq).count() << " ms\n";
    std::cout << "vector:" << std::chrono::duration_cast<milliseconds>(end_rand - start_rand).count() << " ms\n";
    return 0;
}
catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    const char * help = "%s <iteration count>";
    std::printf(help, argv[0]);
    return 1;
}
catch (...)
{
    std::cerr << "Unknown exception\n";
    return 1;
}

