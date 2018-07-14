#pragma once
#include "poly_vector.h"
#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

class Cookie {
public:
    static constexpr uint32_t constructed_cookie = 0xdeadbeef;
    Cookie()
        : cookie {}
    {
    }
    Cookie(const Cookie& other) noexcept
    {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
    }
    Cookie(const Cookie&& other) noexcept
    {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
    }

    Cookie& operator=(const Cookie& other) noexcept
    {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
        return *this;
    }
    Cookie& operator=(const Cookie&& other) noexcept
    {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
        return *this;
    }

    ~Cookie()
    {
        if (cookie != constructed_cookie) {
            std::cerr << "cookie value was " << cookie << std::endl;
        }
        assert(cookie == constructed_cookie);
        cookie = 0;
    }
    void set() noexcept { cookie = 0xdeadbeef; }

private:
    uint32_t cookie;
};
struct Interface {
    explicit Interface(bool throw_on_copy = false)
        : my_id { last_id++ }
        , throw_on_copy_construction { throw_on_copy }
    {
    }
    Interface(const Interface& i)
        : my_id { i.my_id }
        , throw_on_copy_construction { i.throw_on_copy_construction }
    {
        if (i.throw_on_copy_construction)
            throw std::runtime_error("Interface copy attempt with throw on copy set");
    }
    Interface(Interface&& i) noexcept
        : my_id { i.my_id }
        , throw_on_copy_construction { i.throw_on_copy_construction }
    {
        i.my_id = last_id++;
    }
    Interface& operator=(const Interface& i) = delete;

    virtual void       function()                                  = 0;
    virtual Interface* clone(std::allocator<Interface>, void*)     = 0;
    virtual Interface* move(std::allocator<Interface>, void* dest) = 0;
    virtual ~Interface()                                           = default;
    bool   operator<(const Interface& rhs) const noexcept { return my_id < rhs.my_id; }
    bool   operator==(const Interface& rhs) const noexcept { return my_id == rhs.my_id; }
    bool   operator!=(const Interface& rhs) const noexcept { return my_id != rhs.my_id; }
    void   set_throw_on_copy_construction(bool val) noexcept { throw_on_copy_construction = val; }
    size_t getId() const noexcept { return my_id; }

private:
    size_t                     my_id;
    static std::atomic<size_t> last_id;

protected:
    bool throw_on_copy_construction;
};

class Impl1 : public Interface {
public:
    explicit Impl1(double dd = 0.0, bool throw_on_copy = false)
        : Interface { throw_on_copy }
        , d { dd }
        , p { new int[111] }
    {
        cookie.set();
    }
    Impl1(const Impl1& i)
        : Interface(i)
        , d { i.d }
        , p { new int[111] }
    {
        cookie.set();
    }
    Impl1(Impl1&& o) noexcept
        : Interface(std::move(o))
        , d { o.d }
        , p { new int[111] }
    {
        cookie.set();
    }
    void               function() override { d += 1.0; }
    virtual Interface* clone(std::allocator<Interface>, void* dest) override
    {
        return new (dest) Impl1(*this);
    }
    virtual Interface* move(std::allocator<Interface>, void* dest) override
    {
        if (this->throw_on_copy_construction)
            throw std::runtime_error("Interface copy attempt with throw on copy set");
        return new (dest) Impl1(std::move(*this));
    }

private:
    double                 d;
    std::unique_ptr<int[]> p;
    Cookie                 cookie;
};

template <typename cloningp>
class alignas(alignof(std::max_align_t) * 4) Impl2T : public Interface {
public:
    explicit Impl2T(bool throw_on_copy = false)
        : Interface { throw_on_copy }
        , p { new double[137] }
    {
        cookie.set();
    }
    Impl2T(const Impl2T& o)
        : Interface(o)
        , p { new double[137] }
    {
        cookie.set();
    }

    Impl2T(Impl2T&& o) noexcept
        : v { std::move(o.v) }
        , p { std::move(o.p) }
    {
        cookie.set();
    }
    void               function() override { v.push_back(Impl1 { 3.1 }); }
    virtual Interface* clone(std::allocator<Interface>, void* dest) override
    {
        return new (dest) Impl2T(*this);
    }
    virtual Interface* move(std::allocator<Interface>, void* dest) override
    {
        if (this->throw_on_copy_construction)
            throw std::runtime_error("Interface copy attempt with throw on copy set");
        return new (dest) Impl2T(std::move(*this));
    }

private:
    estd::poly_vector<Interface, std::allocator<Interface>, cloningp> v;
    std::unique_ptr<double[]>                                         p;
    Cookie                                                            cookie;
    int                                                               pad[32];
};

using Impl2 = Impl2T<estd::virtual_cloning_policy<Interface>>;

namespace custom {
struct CustInterface;
class CustOtherImpl;
class CustImpl;
struct IVisitor {
    virtual void visit(const CustImpl&) const      = 0;
    virtual void visit(const CustOtherImpl&) const = 0;
    virtual ~IVisitor()                            = default;
};

struct CustInterface {
public:
    virtual void accept(const IVisitor&) const = 0;
    virtual int  doSomething() const           = 0;
    virtual ~CustInterface()                   = default;
};

class CustImpl : public CustInterface {
public:
    int  doSomething() const override { return 42; }
    void accept(const IVisitor& v) const override { v.visit(*this); }
};

class CustOtherImpl : public CustInterface {
public:
    int  doSomething() const override { return 43; }
    void accept(const IVisitor& v) const override { v.visit(*this); }
};

class CloneVisitor : public IVisitor {
public:
    CloneVisitor(void* dst_)
        : dst(dst_)
    {
    }
    void visit(const CustImpl& impl) const override { ptr = new (dst) CustImpl(impl); }
    void visit(const CustOtherImpl& impl) const override { ptr = new (dst) CustOtherImpl(impl); }

    ~CloneVisitor()
    {
        if (ptr)
            ptr->~CustInterface();
    }

    CustInterface* release() const { return std::exchange(ptr, nullptr); }

private:
    mutable void*          dst;
    mutable CustInterface* ptr = nullptr;
};

template <class IF, class Allocator = std::allocator<IF>> struct CustomCloningPolicyT {
    using noexcept_movable = std::false_type;
    using void_pointer     = typename std::allocator_traits<Allocator>::void_pointer;
    using pointer          = typename std::allocator_traits<Allocator>::pointer;
    using allocator_type   = Allocator;
    CustomCloningPolicyT() = default;
    template <typename T> CustomCloningPolicyT(estd::type_tag<T>) {};
    pointer clone(const Allocator& a, pointer obj, void_pointer dest) const
    {
        CloneVisitor v(dest);
        obj->accept(v);
        return v.release();
    }
};

using CustomCloningPolicy = CustomCloningPolicyT<CustInterface>;

template <typename T> struct Allocator : public std::allocator<T> {
    using is_always_equal = std::false_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T        value_type;
    using std::allocator<T>::allocator;
};

}
