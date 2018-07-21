#include <array>
#include <iostream>
#include <poly_vector.h>

struct Interface {
    virtual void doSomething() = 0;
    virtual ~Interface()       = default;
};

class ImplA : public Interface {
public:
    explicit ImplA(double _d = 0.0)
        : d { _d }
    {
    }
    void doSomething() override { std::cout << "ImplA:" << d << '\n'; }

private:
    double d;
};

template <size_t N> struct ImplB : public Interface {
    void                doSomething() override { std::cout << "ImplB:" << N << '\n'; }
    std::array<char, N> arr;
};

struct ImplC : public Interface {
    void doSomething() override { std::cout << "ImplC\n"; }
};

int main()
{
    using poly::poly_vector;

    poly_vector<Interface> v;

    v.push_back(ImplA(3.14));
    v.emplace_back<ImplB<128>>();
    v.emplace_back<ImplC>();

    for (Interface& i : v) {
        // Invoke doSomething on all objects
        i.doSomething();
    }

    // remove the last element
    v.pop_back();

    // invoke doSomething() on ImplB object
    v.back().doSomething();

    // remove the first elem
    v.erase(v.begin());

    // invoke doSomething() on ImplB object (again)
    v.front().doSomething();
}
