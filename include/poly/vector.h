// Copyright (c) 2016 Ferenc Nandor Janky
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include <poly/detail/vector_impl.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace poly {
template <typename T> struct type_tag {
    using type = T;
};

namespace vector_impl {

    // namespace concepts {
    //    using namespace poly;
    //    using namespace std;

    //    template <typename InterfaceT, typename Allocator>
    //    constexpr auto AllocatorPointerMatch = is_same_v<InterfaceT,
    //        typename pointer_traits<typename allocator_traits<Allocator>::pointer>::element_type>;

    //    template <typename T, typename InterfaceT, typename Allocator>
    //    concept HasClone = requires(T cp, Allocator a)
    //    {
    //        {
    //            cp.clone(a, declval<typename allocator_traits<Allocator>::pointer>(),
    //                declval<typename allocator_traits<Allocator>::void_pointer>())
    //        }
    //        ->same_as<typename allocator_traits<Allocator>::pointer>;
    //    };

    //    template <typename T, typename InterfaceT, typename Allocator>
    //    concept HasMove
    //        = is_nothrow_move_constructible_v<T>&& is_nothrow_move_assignable_v<T>&& requires(
    //            T cp, Allocator a)
    //    {
    //        {
    //            cp.move(a, declval<typename allocator_traits<Allocator>::pointer>(),
    //                declval<typename allocator_traits<Allocator>::void_pointer>())
    //        }
    //        ->same_as<typename allocator_traits<Allocator>::pointer>;
    //    };

    //    template <typename T, typename I, typename A, typename Derived>
    //    concept CloningPolicy
    //        = AllocatorPointerMatch<I, A>&& is_nothrow_constructible_v<T>&&
    //              is_nothrow_copy_constructible_v<T>&& is_nothrow_copy_assignable_v<T>&& copyable<
    //                  T> && (constructible_from<T, type_tag<Derived>> ||
    //                  default_initializable<T>)&&(HasClone<T, I, A> || HasMove<T, I, A>);
    //}

    template <typename T> struct TD;

    template <class TT> struct has_noexcept_movable {
        template <class U>
        static std::true_type  test(U&&, typename std::decay_t<U>::noexcept_movable*);
        static std::false_type test(...);
        static constexpr bool  value
            = std::is_same<std::true_type, decltype(test(std::declval<TT>(), nullptr))>::value;
    };

    template <class TT, bool HasType> struct is_noexcept_movable {
        using type = typename TT::noexcept_movable;
    };
    template <class TT> struct is_noexcept_movable<TT, false> {
        using type = std::false_type;
    };

    template <class TT>
    using is_noexcept_movable_t =
        typename is_noexcept_movable<TT, has_noexcept_movable<TT>::value>::type;

    template <class T, class IF, class A> struct is_cloning_policy_impl {
        // TODO(fecjanky): add check for is constructible from type_tag
        using void_ptr = typename std::allocator_traits<A>::void_pointer;
        using pointer  = typename std::allocator_traits<A>::pointer;

        template <class TT> struct has_clone_impl {
            template <class U>
            static decltype(std::declval<U>().clone(
                std::declval<A>(), std::declval<pointer>(), std::declval<void_ptr>()))*
            test(U&&);

            static std::false_type test(...);

            static constexpr bool value
                = std::is_same<decltype(test(std::declval<TT>())), pointer*>::value;
        };
        template <class TT> struct has_move_impl {
            template <class U>
            static decltype(std::declval<U>().move(
                std::declval<A>(), std::declval<pointer>(), std::declval<void_ptr>()))*
                                   test(U&&);
            static std::false_type test(...);
            static constexpr bool  value
                = std::is_same<pointer*, decltype(test(std::declval<TT>()))>::value;
        };

        using has_move_t  = std::integral_constant<bool, has_move_impl<T>::value>;
        using has_clone_t = std::integral_constant<bool, has_clone_impl<T>::value>;
        // TODO(fecjanky): provide traits defaults for Allocator type, pointer
        // and void_pointer types
        static constexpr bool value
            = std::is_same<IF, typename std::pointer_traits<pointer>::element_type>::value
            && std::is_nothrow_constructible<T>::value
            && std::is_nothrow_copy_constructible<T>::value
            && std::is_nothrow_copy_assignable<T>::value
            && (has_clone_t::value || (has_move_t::value && is_noexcept_movable_t<T>::value));
    };

    template <class _Allocator> struct allocator_base : public _Allocator {

        using allocator_type   = _Allocator;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using value_type       = typename allocator_traits::value_type;
        using propagate_on_container_copy_assignment =
            typename allocator_traits::propagate_on_container_copy_assignment;
        using propagate_on_container_swap = typename allocator_traits::propagate_on_container_swap;
        using propagate_on_container_move_assignment =
            typename allocator_traits::propagate_on_container_move_assignment;
        using allocator_is_always_equal = vector_impl::allocator_is_always_equal_t<allocator_type>;
        static_assert(std::is_same<value_type, uint8_t>::value, "requires a byte allocator");
        using void_pointer       = typename allocator_traits::void_pointer;
        using const_void_pointer = typename allocator_traits::const_void_pointer;
        using difference_type    = typename allocator_traits::difference_type;
        using pointer            = typename allocator_traits::pointer;
        using const_pointer      = typename allocator_traits::const_pointer;
        //////////////////////////////////////////////
        explicit allocator_base(const allocator_type& a = allocator_type())
            : allocator_type(a)
            , _storage {}
            , _end_storage {}
        {
        }

        explicit allocator_base(size_t n, const allocator_type& a = allocator_type())
            : allocator_type(a)
            , _storage {}
            , _end_storage {}
        {
            auto s       = get_allocator_ref().allocate(n);
            _storage     = s;
            _end_storage = s + n;
        }

        allocator_base(const allocator_base& a)
            : allocator_base(
                allocator_traits::select_on_container_copy_construction(a.get_allocator_ref()))
        {
            if (a.size()) {
                allocate(a.size());
            }
        }

        allocator_base(allocator_base&& a) noexcept
            : allocator_type(std::move(a.get_allocator_ref()))
            , _storage { a._storage }
            , _end_storage { a._end_storage }
        {
            a._storage = a._end_storage = nullptr;
        }

        allocator_base& operator=(const allocator_base& a)
        {
            return copy_assign_impl(a, propagate_on_container_copy_assignment {});
        }

        allocator_base& operator=(allocator_base&& a) = delete;

        allocator_base& copy_assign_impl(const allocator_base& a, std::true_type /*unused*/)
        {
            static_assert(std::is_nothrow_copy_assignable<allocator_type>::value,
                "allocator copy assignment must not throw");
            pointer s      = nullptr;
            auto    a_copy = a.get_allocator_ref();
            if (a.size()) {
                s = a_copy.allocate(a.size());
            }
            tidy();
            get_allocator_ref() = std::move(a_copy);
            _storage            = s;
            _end_storage        = s + a.size();
            return *this;
        }

        allocator_base& copy_assign_impl(const allocator_base& a, std::false_type /*unused*/)
        {
            pointer s = nullptr;
            if (a.size()) {
                s = get_allocator_ref().allocate(a.size());
            }
            tidy();
            _storage     = s;
            _end_storage = s + a.size();
            return *this;
        }

        void swap(allocator_base& x) noexcept
        {
            // if (!propagate_on_container_swap::value && !allocator_is_always_equal::value
            //    && get_allocator_ref() != x.get_allocator_ref()) -> Undefined behavior!!!
            swap_with_propagate(x, propagate_on_container_swap {});
        }

        template <typename Propagate>
        void swap_with_propagate(allocator_base& x, Propagate /*unused*/) noexcept
        {
            using std::swap;
            /*constexpr*/ if (Propagate::value) {
                swap(get_allocator_ref(), x.get_allocator_ref());
            }
            swap(_storage, x._storage);
            swap(_end_storage, x._end_storage);
        }

        void allocate(size_t n)
        {
            auto s = get_allocator_ref().allocate(n);
            tidy();
            _storage     = s;
            _end_storage = s + n;
        }

        allocator_type&       get_allocator_ref() noexcept { return *this; }
        const allocator_type& get_allocator_ref() const noexcept { return *this; }

        void tidy() noexcept
        {
            get_allocator_ref().deallocate(static_cast<pointer>(_storage), size());
            _storage = _end_storage = nullptr;
        }

        ~allocator_base() { tidy(); }

        difference_type size() const noexcept
        {
            return static_cast<pointer>(_end_storage) - static_cast<pointer>(_storage);
        }

        void_pointer                         storage() noexcept { return _storage; }
        void_pointer                         end_storage() noexcept { return _end_storage; }
        const_void_pointer                   storage() const noexcept { return _storage; }
        const_void_pointer                   end_storage() const noexcept { return _end_storage; }
        template <typename PointerType> void destroy(PointerType obj) const
        {
            using T      = std::decay_t<decltype(*obj)>;
            using traits = typename allocator_traits::template rebind_traits<T>;
            static_assert(
                std::is_same<PointerType, typename traits::pointer>::value, "invalid pointer type");
            typename traits::allocator_type a(get_allocator_ref());
            traits::destroy(a, obj);
        }
        template <typename PointerType, typename... Args>
        PointerType construct(PointerType storage, Args&&... args) const
        {
            using T      = std::decay_t<decltype(*storage)>;
            using traits = typename allocator_traits::template rebind_traits<T>;
            static_assert(
                std::is_same<PointerType, typename traits::pointer>::value, "invalid pointer type");
            typename traits::allocator_type a(get_allocator_ref());
            traits::construct(a, storage, std::forward<Args>(args)...);
            return storage;
        }
        ////////////////////////////////
        void_pointer _storage;
        void_pointer _end_storage;
    };

    template <class Policy, class Interface, class Allocator>
    using is_cloning_policy = std::integral_constant<bool,
        vector_impl::is_cloning_policy_impl<Policy, Interface, Allocator>::value>;

    template <class Policy, class Interface, class Allocator> struct cloning_policy_traits {
        static_assert(is_cloning_policy<Policy, Interface, Allocator>::value,
            "policy is not a cloning policy");
        using allocator_type   = Allocator;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using pointer          = typename allocator_traits::pointer;
        using void_pointer     = typename allocator_traits::void_pointer;

        using policy_impl
            = ::poly::vector_impl::is_cloning_policy_impl<Policy, Interface, allocator_type>;
        using has_move_t       = typename policy_impl::has_move_t;
        using noexcept_movable = std::integral_constant<bool,
            vector_impl::is_noexcept_movable_t<Policy>::value && has_move_t::value>;

        static pointer move(const Policy& p, const allocator_type& a, pointer obj,
            void_pointer dest) noexcept(noexcept_movable::value)
        {
            return move_impl(p, a, obj, dest, typename policy_impl::has_move_t {});
        }

    private:
        static pointer move_impl(const Policy& p, const allocator_type& a, pointer obj,
            void_pointer dest, std::true_type /*unused*/) noexcept(noexcept_movable::value)
        {
            return p.move(a, obj, dest);
        }
        static pointer move_impl(const Policy& p, const allocator_type& a, pointer obj,
            void_pointer dest, std::false_type /*unused*/)
        {
            return p.clone(a, obj, dest);
        }
    };

} // namespace vector_impl

template <class Interface, class Allocator = std::allocator<Interface>,
    typename Interface_Is_NoExcept_Movable = std::true_type>
struct delegate_cloning_policy;

template <class IF, class Allocator = std::allocator<IF>,
    /// implicit noexcept_movability when using defaults of delegate cloning
    /// policy
    class CloningPolicy = delegate_cloning_policy<IF, Allocator>>
class vector;

template <typename CP, typename Constructible> struct CloningPolicyHolder : public CP {
    CloningPolicyHolder()                           = default;
    CloningPolicyHolder(const CloningPolicyHolder&) = default;
    template <typename T> explicit CloningPolicyHolder(type_tag<T> /*unused*/) noexcept { }
};

template <typename CP> struct CloningPolicyHolder<CP, std::true_type> : public CP {
    CloningPolicyHolder()                           = default;
    CloningPolicyHolder(const CloningPolicyHolder&) = default;
    template <typename T>
    explicit CloningPolicyHolder(type_tag<T> t) noexcept
        : CP(t)
    {
    }
};

template <class CloningPolicy, class AllocatorTraits>
struct vector_elem_ptr : private CloningPolicyHolder<CloningPolicy,
                             typename std::is_constructible<CloningPolicy,
                                 type_tag<typename AllocatorTraits::value_type>>::type> {
    using void_pointer  = typename AllocatorTraits::void_pointer;
    using pointer       = typename AllocatorTraits::pointer;
    using const_pointer = typename AllocatorTraits::const_pointer;
    using value_type    = typename AllocatorTraits::value_type;

    using base = CloningPolicyHolder<CloningPolicy,
        typename std::is_constructible<CloningPolicy, type_tag<value_type>>::type>;

    template <typename T>
    using forward_tag_t = typename std::is_constructible<CloningPolicy, T>::type;

    template <typename T>
    using forward_tag = typename std::conditional<forward_tag_t<T>::value, type_tag<T>, void>::type;

    using size_descr_t = std::pair<size_t, size_t>;
    using size_func_t  = size_descr_t();
    using policy_t     = CloningPolicy;
    vector_elem_ptr()
        : ptr {}
        , sf {}
    {
    }

    template <typename T,
        typename = std::enable_if_t<std::is_base_of<value_type, std::decay_t<T>>::value>>
    explicit vector_elem_ptr(type_tag<T> t, void_pointer s = nullptr, pointer i = nullptr) noexcept
        : base(t)
        , ptr { s, i }
        , sf { vector_elem_ptr::size_func<std::decay_t<T>>() }
    {
    }

    vector_elem_ptr(const vector_elem_ptr& other) noexcept
        : base(other)
        , ptr { other.ptr }
        , sf { other.sf }
    {
    }
    vector_elem_ptr& operator=(const vector_elem_ptr& rhs) noexcept
    {
        policy() = rhs.policy();
        ptr      = rhs.ptr;
        sf       = rhs.sf;
        return *this;
    }
    void swap(vector_elem_ptr& rhs) noexcept
    {
        using std::swap;
        swap(policy(), rhs.policy());
        swap(ptr, rhs.ptr);
        swap(sf, rhs.sf);
    }

    policy_t&                                 policy() noexcept { return *this; }
    const policy_t&                           policy() const noexcept { return *this; }
    size_t                                    size() const noexcept { return sf.first; }
    size_t                                    align() const noexcept { return sf.second; }
    template <typename T> static size_descr_t size_func() { return { sizeof(T), alignof(T) }; }
    ~vector_elem_ptr()
    {
        ptr.first = ptr.second = nullptr;
        sf                     = size_descr_t { 0, 0 };
    }

    std::pair<void_pointer, pointer> ptr;

private:
    size_descr_t sf;
};

template <class C, class A>
void swap(vector_elem_ptr<C, A>& lhs, vector_elem_ptr<C, A>& rhs) noexcept
{
    lhs.swap(rhs);
}

struct virtual_cloning_policy {

    template <typename Allocator, typename Pointer, typename VoidPointer>
    Pointer clone(const Allocator& a, Pointer obj, VoidPointer dest) const
    {
        return obj->clone(a, dest);
    }

    template <typename Allocator, typename Pointer, typename VoidPointer>
    Pointer move(const Allocator& a, Pointer obj, VoidPointer dest) const noexcept(noexcept(
        std::declval<Pointer>()->move(std::declval<Allocator>(), std::declval<VoidPointer>())))
    {
        return obj->move(a, dest);
    }
};

struct no_cloning_exception : public std::exception {
    const char* what() const noexcept override { return "cloning attempt with no_cloning_policy"; }
};

struct no_cloning_policy {
    template <typename Allocator, typename Pointer, typename VoidPointer>
    Pointer clone(const Allocator& /* a */, Pointer /* obj */, VoidPointer /* dest */) const
    {
        throw no_cloning_exception {};
    }
};

template <class Interface, class Allocator, typename Interface_Is_NoExcept_Movable>
struct delegate_cloning_policy {
    enum Operation { Clone, Move };

    using clone_func_t     = Interface*(const Allocator& a, Interface* obj, void* dest, Operation);
    using clone_func_ptr_t = clone_func_t*;
    using noexcept_movable = Interface_Is_NoExcept_Movable;
    using void_pointer     = typename std::allocator_traits<Allocator>::void_pointer;
    using pointer          = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer    = typename std::allocator_traits<Allocator>::const_pointer;
    using allocator_type   = Allocator;

    delegate_cloning_policy() noexcept
        : cf {}
    {
    }

    template <typename T,
        typename = std::enable_if_t<!std::is_same<delegate_cloning_policy, std::decay_t<T>>::value>>
    explicit delegate_cloning_policy(type_tag<T> /*unused*/) noexcept
        : cf(&delegate_cloning_policy::clone_func<std::decay_t<T>>)
    {
        // this ensures, that if policy requires noexcept move construction that
        // a given T type also has it
        static_assert(
            !noexcept_movable::value || std::is_nothrow_move_constructible<std::decay_t<T>>::value,
            "delegate cloning policy requires noexcept move constructor");
    }

    delegate_cloning_policy(const delegate_cloning_policy& other) noexcept
        : cf { other.cf }
    {
    }

    delegate_cloning_policy& operator=(const delegate_cloning_policy& other) noexcept = default;

    template <class T>
    static pointer clone_func(const Allocator& a, pointer obj, void_pointer dest, Operation op)
    {
        using traits      = typename std::allocator_traits<Allocator>::template rebind_traits<T>;
        using obj_pointer = typename traits::pointer;
        using obj_const_pointer = typename traits::const_pointer;

        typename traits::allocator_type alloc(a);
        if (op == Clone) {
            traits::construct(
                alloc, static_cast<obj_pointer>(dest), *static_cast<obj_const_pointer>(obj));
        } else {
            traits::construct(
                alloc, static_cast<obj_pointer>(dest), std::move(*static_cast<obj_pointer>(obj)));
        }
        return static_cast<obj_pointer>(dest);
    }

    pointer clone(const Allocator& a, pointer obj, void_pointer dest) const
    {
        return cf(a, obj, dest, Clone);
    }

    pointer move(const Allocator& a, pointer obj, void_pointer dest) const
        noexcept(noexcept_movable::value)
    {
        return cf(a, obj, dest, Move);
    }
    /////////////////////////
private:
    clone_func_ptr_t cf;
};

template <class ElemPtrT>
class vector_iterator : public std::iterator<std::random_access_iterator_tag,
                            typename std::remove_const_t<ElemPtrT>::value_type> {
public:
    using p_elem         = std::remove_const_t<ElemPtrT>;
    using interface_type = typename p_elem::value_type;
    using elem
        = std::conditional_t<std::is_const<ElemPtrT>::value, std::add_const_t<p_elem>, p_elem>;
    using pointer         = typename p_elem::pointer;
    using const_pointer   = typename p_elem::const_pointer;
    using elem_ptr        = typename std::pointer_traits<pointer>::template rebind<elem>;
    using difference_type = typename std::pointer_traits<elem_ptr>::difference_type;
    using reference       = std::add_lvalue_reference_t<interface_type>;
    using const_reference = std::add_lvalue_reference_t<std::add_const_t<interface_type>>;

    vector_iterator()
        : curr {}
    {
    }
    explicit vector_iterator(elem_ptr p)
        : curr { p }
    {
    }

    template <class T,
        typename = std::enable_if_t<std::is_same<p_elem, T>::value
            && !std::is_same<vector_iterator, vector_iterator<T>>::value>>
    vector_iterator(const vector_iterator<T>& other)
        : curr { other.get() }
    {
    }

    vector_iterator(const vector_iterator&) = default;
    vector_iterator(vector_iterator&&)      = default;
    vector_iterator& operator=(const vector_iterator&) = default;
    vector_iterator& operator=(vector_iterator&&) = default;

    ~vector_iterator() = default;

    void swap(vector_iterator& rhs)
    {
        using std::swap;
        swap(curr, rhs.curr);
    }

    pointer         operator->() noexcept { return curr->ptr.second; }
    reference       operator*() noexcept { return *curr->ptr.second; }
    const_pointer   operator->() const noexcept { return curr->ptr.second; }
    const_reference operator*() const noexcept { return *curr->ptr.second; }

    vector_iterator& operator++() noexcept
    {
        ++curr;
        return *this;
    }
    vector_iterator operator++(int) noexcept
    {
        vector_iterator i(*this);
        ++curr;
        return i;
    }
    vector_iterator& operator--() noexcept
    {
        --curr;
        return *this;
    }
    vector_iterator operator--(int) noexcept
    {
        vector_iterator i(*this);
        --curr;
        return i;
    }
    vector_iterator& operator+=(difference_type n)
    {
        curr += n;
        return *this;
    }
    vector_iterator& operator-=(difference_type n)
    {
        curr -= n;
        return *this;
    }
    difference_type operator-(vector_iterator rhs) const { return curr - rhs.curr; }
    reference       operator[](difference_type n) { return *(curr[n].ptr.second); }
    const_reference operator[](difference_type n) const { return *(curr[n].ptr.second); }
    bool operator==(const vector_iterator& rhs) const noexcept { return curr == rhs.curr; }
    bool operator!=(const vector_iterator& rhs) const noexcept { return curr != rhs.curr; }
    bool operator<(const vector_iterator& rhs) const noexcept { return curr < rhs.curr; }
    bool operator>(const vector_iterator& rhs) const noexcept { return curr > rhs.curr; }
    bool operator<=(const vector_iterator& rhs) const noexcept { return curr <= rhs.curr; }
    bool operator>=(const vector_iterator& rhs) const noexcept { return curr >= rhs.curr; }

    elem_ptr get() const noexcept { return curr; }

private:
    elem_ptr curr;
};

template <class ElemPtrT> void swap(vector_iterator<ElemPtrT>& lhs, vector_iterator<ElemPtrT>& rhs)
{
    lhs.swap(rhs);
}
template <class ElemPtrT>
vector_iterator<ElemPtrT> operator+(
    vector_iterator<ElemPtrT> a, typename vector_iterator<ElemPtrT>::difference_type n)
{
    return a += n;
}
template <class ElemPtrT>
vector_iterator<ElemPtrT> operator+(
    typename vector_iterator<ElemPtrT>::difference_type n, vector_iterator<ElemPtrT> a)
{
    return a += n;
}
template <class ElemPtrT>
vector_iterator<ElemPtrT> operator-(
    vector_iterator<ElemPtrT> i, typename vector_iterator<ElemPtrT>::difference_type n)
{
    return i -= n;
}

template <class IF, class Allocator, class CloningPolicy>
class vector : private vector_impl::allocator_base<
                   typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>> {
public:
    ///////////////////////////////////////////////
    // Member types
    ///////////////////////////////////////////////
    using interface_type = std::decay_t<IF>;
    using allocator_type =
        typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;

#ifdef POLY_VECTOR_MSVC_WORKAROUND
    // NOTE: for some unknown reason if this rebind using declaration is not here
    // Allocator type is the rebound allocator type
    using interface_allocator_type =
        typename std::allocator_traits<Allocator>::template rebind_alloc<IF>;
#else
    using interface_allocator_type = Allocator;
#endif

    using interface_allocator_traits = std::allocator_traits<interface_allocator_type>;
    using allocator_traits           = std::allocator_traits<allocator_type>;
    using my_base                    = vector_impl::allocator_base<allocator_type>;
    using interface_pointer          = typename interface_allocator_traits::pointer;
    using const_interface_pointer    = typename interface_allocator_traits::const_pointer;
    using interface_reference        = std::add_lvalue_reference_t<interface_type>;
    using const_interface_reference = std::add_lvalue_reference_t<std::add_const_t<interface_type>>;
    using pointer                   = typename my_base::pointer;
    using const_pointer             = typename my_base::const_pointer;
    using void_pointer              = typename my_base::void_pointer;
    using const_void_pointer        = typename my_base::const_void_pointer;
    using size_type                 = std::size_t;
    using cloning_policy            = CloningPolicy;
    using elem_ptr                  = vector_elem_ptr<cloning_policy, interface_allocator_traits>;
    using iterator                  = vector_iterator<elem_ptr>;
    using const_iterator            = vector_iterator<elem_ptr const>;
    using reverse_iterator          = std::reverse_iterator<iterator>;
    using const_reverse_iterator    = std::reverse_iterator<const_iterator>;
    using cloning_policy_traits = vector_impl::cloning_policy_traits<CloningPolicy, interface_type,
        interface_allocator_type>;
    using interface_type_noexcept_movable = typename cloning_policy_traits::noexcept_movable;

    static_assert(std::is_same<IF, interface_type>::value,
        "interface type must be a non-cv qualified user defined type");
    static_assert(std::is_polymorphic<interface_type>::value, "interface_type is not polymorphic");
    static_assert(vector_impl::is_cloning_policy<CloningPolicy, interface_type,
                      interface_allocator_type>::value,
        "invalid cloning policy type");
    static_assert(alignof(elem_ptr) <= alignof(std::max_align_t),
        "cloning policy type must not be over aligned");
    static constexpr auto default_avg_size   = 4 * sizeof(void*);
    static constexpr auto default_alignement = alignof(interface_reference);
    ///////////////////////////////////////////////
    // Ctors,Dtors & assignment
    ///////////////////////////////////////////////
    vector();
    explicit vector(const allocator_type& alloc);
    vector(const vector& other);
    vector(vector&& other) noexcept;
    ~vector();

    vector& operator=(const vector& rhs);
    vector& operator=(vector&& rhs) noexcept;
    // TODO: implement construcor, where all T :< IF
    // explicit vector(T&&...);
    ///////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////
    template <typename T>
    std::enable_if_t<std::is_base_of<interface_type, std::decay_t<T>>::value, void> push_back(
        T&& obj);

    template <typename T, typename... Args>
    std::enable_if_t<std::is_base_of<interface_type, T>::value, interface_reference> emplace_back(
        Args&&... args);

    void pop_back() noexcept;
    void clear() noexcept;
    void swap(vector& x) noexcept;

    // TODO(fecja): insert, erase,emplace, emplace_back, assign?
    template <class descendant_type>
    std::enable_if_t<std::is_base_of<interface_type, std::decay_t<descendant_type>>::value,
        iterator>
    insert(const_iterator position, descendant_type&& val);
    // template <class InputIterator>
    // iterator insert (const_iterator position, InputIterator first,
    // InputIterator last); iterator insert (const_iterator position,
    // polyvectoriterator first, polyvectoriterator last);
    iterator erase(const_iterator position);
    iterator erase(const_iterator first, const_iterator last);
    ///////////////////////////////////////////////
    // Iterators
    ///////////////////////////////////////////////
    iterator               begin() noexcept;
    iterator               end() noexcept;
    const_iterator         begin() const noexcept;
    const_iterator         end() const noexcept;
    reverse_iterator       rbegin() noexcept;
    reverse_iterator       rend() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator rend() const noexcept;
    const_iterator         cbegin() const noexcept;
    const_iterator         cend() const noexcept;
    ///////////////////////////////////////////////
    // Capacity
    ///////////////////////////////////////////////
    size_t                    size() const noexcept;
    std::pair<size_t, size_t> sizes() const noexcept;
    size_type                 capacity() const noexcept;
    std::pair<size_t, size_t> capacities() const noexcept;
    bool                      empty() const noexcept;
    size_type                 max_size() const noexcept;
    size_type                 max_align() const noexcept;
    void reserve(size_type n, size_type avg_size, size_type max_align = alignof(std::max_align_t));
    void reserve(size_type n);
    void reserve(std::pair<size_t, size_t> s);
    // TODO(fecjanky): void shrink_to_fit();
    ///////////////////////////////////////////////
    // Element access
    ///////////////////////////////////////////////
    interface_reference       operator[](size_t n) noexcept;
    const_interface_reference operator[](size_t n) const noexcept;
    interface_reference       at(size_t n);
    const_interface_reference at(size_t n) const;
    interface_reference       front() noexcept;
    const_interface_reference front() const noexcept;
    interface_reference       back() noexcept;
    const_interface_reference back() const noexcept;

    std::pair<void_pointer, void_pointer>             data() noexcept;
    std::pair<const_void_pointer, const_void_pointer> data() const noexcept;
    ////////////////////////////
    // Misc.
    ////////////////////////////
    allocator_type get_allocator() const noexcept;

private:
    template <typename T> using type_tag = type_tag<T>;

    using elem_ptr_pointer = typename allocator_traits::template rebind_traits<elem_ptr>::pointer;
    using elem_ptr_const_pointer =
        typename allocator_traits::template rebind_traits<elem_ptr>::const_pointer;
    using poly_copy_descr = std::tuple<elem_ptr_pointer, elem_ptr_pointer, void_pointer>;

    ////////////////////////
    /// Storage management helpers
    ////////////////////////
    static void_pointer       next_aligned_storage(void_pointer p, size_t align) noexcept;
    static size_t             storage_size(const_void_pointer b, const_void_pointer e) noexcept;
    std::pair<size_t, size_t> calculate_storage_size(
        size_t new_size, size_t new_elem_size, size_t new_alignment) const noexcept;
    size_type occupied_storage(elem_ptr_const_pointer p) const noexcept;

    template <typename CopyOrMove>
    void increase_storage(
        size_t desired_size, size_t curr_elem_size, size_t align, CopyOrMove /*unused*/);

    void obtain_storage(my_base&& a, size_t n, size_t max_align, std::true_type /*unused*/);

    void obtain_storage(
        my_base&& a, size_t n, size_t max_align, std::false_type /*unused*/) noexcept;

    void init_layout(size_t storage_size, size_t capacity, size_t align_max = default_alignement);

    my_base&                              base() noexcept;
    const my_base&                        base() const noexcept;
    static poly_copy_descr                poly_uninitialized_copy(my_base& a, void_pointer dst_ptr,
                       elem_ptr_const_pointer begin, elem_ptr_const_pointer _free, elem_ptr_const_pointer end,
                       size_t max_align);
    static poly_copy_descr                poly_uninitialized_move(my_base& a, void_pointer dst_ptr,
                       elem_ptr_const_pointer begin, elem_ptr_const_pointer _free, elem_ptr_const_pointer end,
                       size_t max_align) noexcept;
    vector&                               copy_assign_impl(const vector& rhs);
    vector&                               move_assign_impl(vector&& rhs) noexcept;
    void                                  tidy() noexcept;
    void                                  destroy_elem(elem_ptr_pointer p) noexcept;
    std::pair<void_pointer, void_pointer> destroy_range(
        elem_ptr_pointer first, elem_ptr_pointer last) noexcept;
    iterator erase_internal_range(elem_ptr_pointer first, elem_ptr_pointer last);
    iterator clear_till_end(elem_ptr_pointer first) noexcept;
    void     init_ptrs(size_t cap) noexcept;
    void     set_ptrs(poly_copy_descr p);
    void     swap_ptrs(vector&& rhs);
    template <class T, typename... Args> void push_back_new_elem(type_tag<T> t, Args&&... args);
    template <class T, typename... Args>
    void         push_back_new_elem_w_storage_increase(type_tag<T> t, Args&&... args);
    void         push_back_new_elem_w_storage_increase_copy(vector& v, std::true_type /*unused*/);
    void         push_back_new_elem_w_storage_increase_copy(vector& v, std::false_type /*unused*/);
    bool         can_construct_new_elem(size_t s, size_t align) noexcept;
    void_pointer next_aligned_storage(size_t align) const noexcept;
    size_t       avg_obj_size(size_t align = 1) const noexcept;
    elem_ptr_pointer       begin_elem() noexcept;
    elem_ptr_const_pointer cbegin_elem() const noexcept;
    elem_ptr_pointer       end_elem() noexcept;
    elem_ptr_const_pointer begin_elem() const noexcept;
    elem_ptr_const_pointer end_elem() const noexcept;
    elem_ptr_const_pointer last_elem() const noexcept;
    void_pointer           free_storage() const noexcept;
    ////////////////////////////
    // Members
    ////////////////////////////
    elem_ptr_pointer _free_elem;
    void_pointer     _begin_storage;
    size_t           _align_max;
};

template <class IF, class Allocator, class CloningPolicy>
void swap(
    vector<IF, Allocator, CloningPolicy>& lhs, vector<IF, Allocator, CloningPolicy>& rhs) noexcept
{
    lhs.swap(rhs);
}
// TODO(fecja): relational operators

/////////////////////////
// implementation
////////////////////////
template <class I, class A, class C>
inline vector<I, A, C>::vector()
    : _free_elem {}
    , _begin_storage {}
    , _align_max { default_alignement }
{
}

template <class I, class A, class C>
inline vector<I, A, C>::vector(const allocator_type& alloc)
    : vector_impl::allocator_base<allocator_type>(alloc)
    , _free_elem {}
    , _begin_storage {}
    , _align_max { default_alignement }
{
}

template <class I, class A, class C>
inline vector<I, A, C>::vector(const vector& other)
    : vector_impl::allocator_base<allocator_type>(other.base())
    , _free_elem { begin_elem() }
    , _begin_storage { begin_elem() + other.capacity() }
    , _align_max { other._align_max }
{
    set_ptrs(poly_uninitialized_copy(base(), begin_elem(), other.begin_elem(), other.end_elem(),
        other.last_elem(), other.max_align()));
}

template <class I, class A, class C>
inline vector<I, A, C>::vector(vector&& other) noexcept
    : vector_impl::allocator_base<allocator_type>(std::move(other.base()))
    , _free_elem { other._free_elem }
    , _begin_storage { other._begin_storage }
    , _align_max { other._align_max }
{
    other._begin_storage = other._free_elem = nullptr;
    other._align_max                        = default_alignement;
}

template <class I, class A, class C> inline vector<I, A, C>::~vector() { tidy(); }

template <class I, class A, class C>
inline vector<I, A, C>& vector<I, A, C>::operator=(const vector& rhs)
{
    if (this != &rhs) {
        copy_assign_impl(rhs);
    }
    return *this;
}

template <class I, class A, class C>
inline vector<I, A, C>& vector<I, A, C>::operator=(vector&& rhs) noexcept
{
    if (this != &rhs) {
        move_assign_impl(std::move(rhs));
    }
    return *this;
}

template <class I, class A, class C>
template <typename T>
inline auto vector<I, A, C>::push_back(T&& obj)
    -> std::enable_if_t<std::is_base_of<interface_type, std::decay_t<T>>::value>
{
    using TT         = std::decay_t<T>;
    constexpr auto s = sizeof(TT);
    constexpr auto a = alignof(TT);
    if (!can_construct_new_elem(s, a)) {
        push_back_new_elem_w_storage_increase(type_tag<TT> {}, std::forward<T>(obj));
    } else {
        push_back_new_elem(type_tag<TT> {}, std::forward<T>(obj));
    }
}

template <class IF, class Allocator, class CloningPolicy>
template <typename T, typename... Args>
inline auto vector<IF, Allocator, CloningPolicy>::emplace_back(Args&&... args)
    -> std::enable_if_t<std::is_base_of<interface_type, T>::value, interface_reference>
{
    constexpr auto s = sizeof(T);
    constexpr auto a = alignof(T);
    if (!can_construct_new_elem(s, a)) {
        push_back_new_elem_w_storage_increase(type_tag<T> {}, std::forward<Args>(args)...);
    } else {
        push_back_new_elem(type_tag<T> {}, std::forward<Args>(args)...);
    }
    return back();
}

template <class I, class A, class C> inline void vector<I, A, C>::pop_back() noexcept
{
    clear_till_end(_free_elem - 1);
}

template <class I, class A, class C> inline void vector<I, A, C>::clear() noexcept
{
    clear_till_end(begin_elem());
    _align_max = default_alignement;
}

template <class I, class A, class C> inline void vector<I, A, C>::swap(vector& x) noexcept
{
    using std::swap;
    base().swap(x.base());
    swap(_free_elem, x._free_elem);
    swap(_begin_storage, x._begin_storage);
    swap(_align_max, x._align_max);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::erase(const_iterator position) -> iterator
{
    return erase(position, position + 1);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::erase(const_iterator first, const_iterator last) -> iterator
{
    auto eptr_first = begin_elem() + (first - begin());
    auto ret        = last == end() ? clear_till_end(eptr_first)
                                    : erase_internal_range(eptr_first, begin_elem() + (last - begin()));
    if (empty()) {
        _align_max = default_alignement;
    }
    return ret;
}

template <class I, class A, class C> inline auto vector<I, A, C>::begin() noexcept -> iterator
{
    return iterator(begin_elem());
}

template <class I, class A, class C> inline auto vector<I, A, C>::end() noexcept -> iterator
{
    return iterator(end_elem());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::begin() const noexcept -> const_iterator
{
    return const_iterator(begin_elem());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::end() const noexcept -> const_iterator
{
    return const_iterator(end_elem());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::rbegin() noexcept -> reverse_iterator
{
    return std::make_reverse_iterator(end());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::rend() noexcept -> reverse_iterator
{
    return std::make_reverse_iterator(begin());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::rbegin() const noexcept -> const_reverse_iterator
{
    return std::make_reverse_iterator(end());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::rend() const noexcept -> const_reverse_iterator
{
    return std::make_reverse_iterator(begin());
}

template <class IF, class Allocator, class CloningPolicy>
inline auto vector<IF, Allocator, CloningPolicy>::cbegin() const noexcept -> const_iterator
{
    return begin();
}

template <class IF, class Allocator, class CloningPolicy>
inline auto vector<IF, Allocator, CloningPolicy>::cend() const noexcept -> const_iterator
{
    return end();
}

template <class I, class A, class C> inline size_t vector<I, A, C>::size() const noexcept
{
    return static_cast<size_t>(_free_elem - begin_elem());
}

template <class I, class A, class C>
inline std::pair<size_t, size_t> vector<I, A, C>::sizes() const noexcept
{
    return std::make_pair(size(), avg_obj_size());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::capacity() const noexcept -> size_type
{
    return storage_size(begin_elem(), _begin_storage) / sizeof(elem_ptr);
}

template <class I, class A, class C>
inline std::pair<size_t, size_t> vector<I, A, C>::capacities() const noexcept
{
    return std::make_pair(capacity(), storage_size(_begin_storage, this->_end_storage));
}

template <class I, class A, class C> inline bool vector<I, A, C>::empty() const noexcept
{
    return begin_elem() == _free_elem;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::max_size() const noexcept -> size_type
{
    auto avg = avg_obj_size() ? avg_obj_size() : 4 * sizeof(void_pointer);
    return allocator_traits::max_size(this->get_allocator_ref()) / (sizeof(elem_ptr) + avg);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::max_align() const noexcept -> size_type
{
    return _align_max;
}

template <class I, class A, class C>
inline void vector<I, A, C>::reserve(size_type n, size_type avg_size, size_type max_align)
{
    using copy = std::conditional_t<interface_type_noexcept_movable::value, std::false_type,
        std::true_type>;
    if (n <= capacities().first && avg_size <= capacities().second && _align_max >= max_align) {
        return;
    }
    if (n > max_size()) {
        throw std::length_error("poly::vector reserve size too big");
    }
    increase_storage(n, avg_size, max_align, copy {});
}

template <class I, class A, class C> inline void vector<I, A, C>::reserve(size_type n)
{
    reserve(n, default_avg_size);
}

template <class I, class A, class C>
inline void vector<I, A, C>::reserve(std::pair<size_t, size_t> s)
{
    reserve(s.first, s.second);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::operator[](size_t n) noexcept -> interface_reference
{
    return *begin_elem()[n].ptr.second;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::operator[](size_t n) const noexcept -> const_interface_reference
{
    return *begin_elem()[n].ptr.second;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::at(size_t n) -> interface_reference
{
    if (n >= size()) {
        throw std::out_of_range { "poly::vector out of range access" };
    };
    return (*this)[n];
}

template <class I, class A, class C>
inline auto vector<I, A, C>::at(size_t n) const -> const_interface_reference
{
    if (n >= size()) {
        throw std::out_of_range { "poly::vector out of range access" };
    };
    return (*this)[n];
}

template <class I, class A, class C>
inline auto vector<I, A, C>::front() noexcept -> interface_reference
{
    return (*this)[0];
}

template <class I, class A, class C>
inline auto vector<I, A, C>::front() const noexcept -> const_interface_reference
{
    return (*this)[0];
}

template <class I, class A, class C>
inline auto vector<I, A, C>::back() noexcept -> interface_reference
{
    return (*this)[size() - 1];
}

template <class I, class A, class C>
inline auto vector<I, A, C>::back() const noexcept -> const_interface_reference
{
    return (*this)[size() - 1];
}

template <class I, class A, class C>
inline auto vector<I, A, C>::data() noexcept -> std::pair<void_pointer, void_pointer>
{
    return std::make_pair(base()._storage, base()._end_storage);
}
template <class I, class A, class C>
inline auto vector<I, A, C>::data() const noexcept
    -> std::pair<const_void_pointer, const_void_pointer>
{
    return std::make_pair(base()._storage, base()._end_storage);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::get_allocator() const noexcept -> allocator_type
{
    return my_base::get_allocator_ref();
}

template <class I, class A, class C>
inline auto vector<I, A, C>::next_aligned_storage(void_pointer p, size_t align) noexcept
    -> void_pointer
{
    auto v = static_cast<pointer>(p) - static_cast<pointer>(nullptr);
    auto a = ((v + align - 1) / align) * align - v;
    return static_cast<pointer>(p) + a;
}

template <class I, class A, class C>
inline size_t vector<I, A, C>::storage_size(const_void_pointer b, const_void_pointer e) noexcept
{
    using const_pointer_t = typename allocator_traits::const_pointer;
    return static_cast<size_t>(static_cast<const_pointer_t>(e) - static_cast<const_pointer_t>(b));
}

template <class I, class A, class C>
template <typename CopyOrMove>
inline void vector<I, A, C>::increase_storage(
    size_t desired_size, size_t curr_elem_size, size_t align, CopyOrMove /*unused*/)
{
    auto    sizes = calculate_storage_size(desired_size, curr_elem_size, align);
    my_base s(sizes.first,
        allocator_traits::select_on_container_copy_construction(base().get_allocator_ref()));
    obtain_storage(std::move(s), desired_size, sizes.second, CopyOrMove {});
}

template <class I, class A, class C>
inline void vector<I, A, C>::obtain_storage(
    my_base&& a, size_t n, size_t max_align, std::true_type /*unused*/)
{
    auto ret = poly_uninitialized_copy(
        a, a.storage(), begin_elem(), begin_elem(), std::next(begin_elem(), n), max_align);
    tidy();
    base().swap(a);
    set_ptrs(ret);
    _align_max = max_align;
}

template <class I, class A, class C>
inline void vector<I, A, C>::obtain_storage(
    my_base&& a, size_t n, size_t max_align, std::false_type /*unused*/) noexcept
{
    _align_max = max_align;
    auto ret   = poly_uninitialized_move(
        a, a.storage(), begin_elem(), end_elem(), std::next(begin_elem(), n), max_align);
    tidy();
    base().swap(a);
    set_ptrs(ret);
    _align_max = max_align;
}

template <class IF, class Allocator, class CloningPolicy>
inline void vector<IF, Allocator, CloningPolicy>::init_layout(
    size_t storage_size, size_t capacity, size_t align_max)
{
    base().allocate(storage_size);
    init_ptrs(capacity);
    _align_max = align_max;
}

template <class I, class A, class C> inline auto vector<I, A, C>::base() noexcept -> my_base&
{
    return *this;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::base() const noexcept -> const my_base&
{
    return *this;
}

template <class IF, class Allocator, class CloningPolicy>
inline auto vector<IF, Allocator, CloningPolicy>::poly_uninitialized_copy(my_base& a,
    void_pointer dst_ptr, elem_ptr_const_pointer begin, elem_ptr_const_pointer _free,
    elem_ptr_const_pointer end, size_t max_align) -> poly_copy_descr
{
    const auto   dst_begin     = static_cast<elem_ptr_pointer>(dst_ptr);
    const auto   storage_begin = dst_begin + std::distance(begin, end);
    auto         dst           = dst_begin;
    void_pointer dst_storage   = storage_begin;
    for (auto elem_dst = dst_begin; elem_dst != storage_begin; ++elem_dst) {
        a.construct(elem_dst);
    }
    try {
        for (auto elem = begin; elem != _free; ++elem, ++dst) {
            *dst           = *elem;
            dst->ptr.first = next_aligned_storage(dst_storage, max_align);
            dst->ptr.second
                = elem->policy().clone(a.get_allocator_ref(), elem->ptr.second, dst->ptr.first);
            dst_storage = static_cast<pointer>(dst->ptr.first) + dst->size();
        }
        return std::make_tuple(dst, storage_begin, dst_storage);
    } catch (...) {
        while (dst-- != dst_begin) {
            a.destroy(dst->ptr.second);
        }
        for (auto elem_dst = dst_begin; elem_dst != storage_begin; ++elem_dst) {
            a.destroy(elem_dst);
        }
        throw;
    }
}

template <class IF, class Allocator, class CloningPolicy>
inline auto vector<IF, Allocator, CloningPolicy>::poly_uninitialized_move(my_base& a,
    void_pointer dst_ptr, elem_ptr_const_pointer begin, elem_ptr_const_pointer _free,
    elem_ptr_const_pointer end, size_t max_align) noexcept -> poly_copy_descr
{
    const auto   dst_begin     = static_cast<elem_ptr_pointer>(dst_ptr);
    auto         dst           = dst_begin;
    const auto   storage_begin = dst_begin + std::distance(begin, end);
    void_pointer dst_storage   = storage_begin;
    for (auto elem_dst = dst_begin; elem_dst != storage_begin; ++elem_dst) {
        a.construct(elem_dst);
    }
    for (auto elem = begin; elem != _free; ++elem, ++dst) {
        *dst            = *elem;
        dst->ptr.first  = next_aligned_storage(dst_storage, max_align);
        dst->ptr.second = cloning_policy_traits::move(
            elem->policy(), a.get_allocator_ref(), elem->ptr.second, dst->ptr.first);
        dst_storage = static_cast<pointer>(dst->ptr.first) + dst->size();
    }
    return std::make_tuple(dst, storage_begin, dst_storage);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::copy_assign_impl(const vector& rhs) -> vector&
{
    tidy();
    base() = rhs.base();
    init_ptrs(rhs.capacity());
    _align_max = rhs._align_max;
    set_ptrs(poly_uninitialized_copy(
        base(), begin_elem(), rhs.begin_elem(), rhs.end_elem(), rhs.last_elem(), rhs.max_align()));
    return *this;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::move_assign_impl(vector&& rhs) noexcept -> vector&
{
    using std::swap;
    base().swap_with_propagate(
        rhs.base(), typename my_base::propagate_on_container_move_assignment {});
    swap_ptrs(std::move(rhs));
    swap(_align_max, rhs._align_max);
    return *this;
}

template <class I, class A, class C> inline void vector<I, A, C>::tidy() noexcept
{
    clear();
    for (auto i = begin_elem(); i != begin_elem() + capacity(); ++i) {
        base().destroy(i);
    }
    _begin_storage = _free_elem = nullptr;
    my_base::tidy();
}

template <class I, class A, class C>
inline void vector<I, A, C>::destroy_elem(elem_ptr_pointer p) noexcept
{
    base().destroy(p->ptr.second);
    *p = elem_ptr();
}

template <class I, class A, class C>
inline auto vector<I, A, C>::destroy_range(elem_ptr_pointer first, elem_ptr_pointer last) noexcept
    -> std::pair<void_pointer, void_pointer>
{
    std::pair<void_pointer, void_pointer> ret {};
    if (first != last) {
        ret.first  = first->ptr.first;
        ret.second = last != end_elem() ? last->ptr.first : base().end_storage();
    }
    for (; first != last; ++first) {
        destroy_elem(first);
    }
    return ret;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::erase_internal_range(elem_ptr_pointer first, elem_ptr_pointer last)
    -> iterator
{
    assert(first != last);
    assert(last != end_elem());
    using std::swap;
    auto return_iterator = first;
    auto free_range      = destroy_range(first, last);
    for (; last != end_elem()
         && occupied_storage(last) <= storage_size(free_range.first, free_range.second);
         ++last, ++first) {
        try {
            auto clone = cloning_policy_traits::move(
                last->policy(), base().get_allocator_ref(), last->ptr.second, free_range.first);
            base().destroy(last->ptr.second);
            swap(*first, *last);
            first->ptr = std::make_pair(free_range.first, clone);
            free_range = std::make_pair(
                next_aligned_storage(
                    static_cast<pointer>(free_range.first) + first->size(), _align_max),
                next_aligned_storage(
                    static_cast<pointer>(free_range.second) + first->size(), _align_max));
        } catch (...) {
            destroy_range(last, end_elem());
            _free_elem = first;
            throw;
        }
    }
    for (; last != end_elem(); ++last, ++first) {
        swap(*first, *last);
    }
    _free_elem = first;
    return iterator(return_iterator);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::clear_till_end(elem_ptr_pointer first) noexcept -> iterator
{
    destroy_range(first, _free_elem);
    _free_elem = first;
    return end();
}

template <class I, class A, class C> inline void vector<I, A, C>::set_ptrs(poly_copy_descr p)
{
    _free_elem     = std::get<0>(p);
    _begin_storage = std::get<1>(p);
}

template <class I, class A, class C> inline void vector<I, A, C>::swap_ptrs(vector&& rhs)
{
    using std::swap;
    swap(_free_elem, rhs._free_elem);
    swap(_begin_storage, rhs._begin_storage);
}

template <class I, class A, class C>
template <class T, typename... Args>
inline void vector<I, A, C>::push_back_new_elem(type_tag<T> /* t */, Args&&... args)
{
    constexpr auto s = sizeof(T);
    constexpr auto a = alignof(T);
    using traits     = typename allocator_traits ::template rebind_traits<T>;

    assert(can_construct_new_elem(s, a));
    assert(_align_max >= a);
    auto nas = next_aligned_storage(_align_max);
    auto obj_ptr
        = base().construct(static_cast<typename traits::pointer>(nas), std::forward<Args>(args)...);
    *_free_elem = elem_ptr(type_tag<T> {}, nas, obj_ptr);
    ++_free_elem;
}

template <class I, class A, class C>
template <class T, typename... Args>
inline void vector<I, A, C>::push_back_new_elem_w_storage_increase(
    type_tag<T> /* t */, Args&&... args)
{
    constexpr auto s                = sizeof(T);
    constexpr auto a                = alignof(T);
    constexpr auto noexcept_movable = interface_type_noexcept_movable::value;
    constexpr auto nothrow_ctor     = std::is_nothrow_constructible<T, Args...>::value;
    //////////////////////////////////////////
    const auto new_capacity = std::max(capacity() * 2, size_t(1));
    const auto sizes        = calculate_storage_size(new_capacity, s, a);
    vector v(allocator_traits::select_on_container_copy_construction(base().get_allocator_ref()));
    v.init_layout(sizes.first, new_capacity, sizes.second);
    push_back_new_elem_w_storage_increase_copy(
        v, std::integral_constant < bool, noexcept_movable&& nothrow_ctor > {});
    v.push_back_new_elem(type_tag<T> {}, std::forward<Args>(args)...);
    this->swap(v);
}

template <class I, class A, class C>
inline void vector<I, A, C>::push_back_new_elem_w_storage_increase_copy(
    vector& v, std::true_type /*unused*/)
{
    v.set_ptrs(poly_uninitialized_move(base(), v.begin_elem(), begin_elem(), end_elem(),
        std::next(begin_elem(), v.capacity()), v.max_align()));
}

template <class I, class A, class C>
inline void vector<I, A, C>::push_back_new_elem_w_storage_increase_copy(
    vector& v, std::false_type /*unused*/)
{
    v.set_ptrs(poly_uninitialized_copy(v.base(), v.begin_elem(), begin_elem(), _free_elem,
        std::next(begin_elem(), v.capacity()), v.max_align()));
}

template <class I, class A, class C>
inline bool vector<I, A, C>::can_construct_new_elem(size_t s, size_t align) noexcept
{
    if (end_elem() == _begin_storage || align > _align_max) {
        return false;
    }
    auto free = static_cast<pointer>(next_aligned_storage(free_storage(), _align_max));
    return free + s <= this->end_storage();
}

template <class I, class A, class C>
inline auto vector<I, A, C>::next_aligned_storage(size_t align) const noexcept -> void_pointer
{
    return next_aligned_storage(free_storage(), align);
}

template <class I, class A, class C>
inline size_t vector<I, A, C>::avg_obj_size(size_t align) const noexcept
{
    return !empty()
        ? (static_cast<size_t>(storage_size(_begin_storage, next_aligned_storage(align))) + size()
              - 1)
            / size()
        : 0;
}

template <class I, class A, class C>
inline auto vector<I, A, C>::begin_elem() noexcept -> elem_ptr_pointer
{
    return static_cast<elem_ptr_pointer>(this->storage());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::cbegin_elem() const noexcept -> elem_ptr_const_pointer
{
    return begin_elem();
}

template <class I, class A, class C>
inline auto vector<I, A, C>::end_elem() noexcept -> elem_ptr_pointer
{
    return static_cast<elem_ptr_pointer>(_free_elem);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::begin_elem() const noexcept -> elem_ptr_const_pointer
{
    return static_cast<elem_ptr_const_pointer>(this->storage());
}

template <class I, class A, class C>
inline auto vector<I, A, C>::end_elem() const noexcept -> elem_ptr_const_pointer
{
    return static_cast<elem_ptr_const_pointer>(_free_elem);
}

template <class IF, class Allocator, class CloningPolicy>
inline auto vector<IF, Allocator, CloningPolicy>::last_elem() const noexcept
    -> elem_ptr_const_pointer
{
    return static_cast<elem_ptr_const_pointer>(_begin_storage);
}

template <class IF, class Allocator, class CloningPolicy>
inline auto vector<IF, Allocator, CloningPolicy>::free_storage() const noexcept -> void_pointer
{
    if (_free_elem == this->storage()) {
        return this->_begin_storage;
    }
    const auto prev_elem = std::prev(_free_elem);
    return static_cast<pointer>(prev_elem->ptr.first) + prev_elem->size();
}

template <class I, class A, class C> inline void vector<I, A, C>::init_ptrs(size_t cap) noexcept
{
    _free_elem     = begin_elem();
    _begin_storage = begin_elem() + cap;
}

template <class I, class A, class C>
inline std::pair<size_t, size_t> vector<I, A, C>::calculate_storage_size(
    size_t new_size, size_t new_elem_size, size_t new_alignment) const noexcept
{
    const auto max_alignment            = std::max(new_alignment, max_align());
    const auto initial_alignment_buffer = max_alignment;
    const auto new_object_size
        = ((new_elem_size + max_alignment - 1) / max_alignment) * max_alignment;
    const auto buffer_size = std::accumulate(begin_elem(), end_elem(), initial_alignment_buffer,
        [max_alignment](size_t val, const auto& p) {
            return val + ((p.size() + max_alignment - 1) / max_alignment) * max_alignment;
        });
    auto       avg_obj_size
        = !empty() ? (buffer_size + new_object_size + size()) / (size() + 1) : new_object_size;
    auto num_of_new_obj = new_size >= size() ? (new_size - size()) : 0U;
    auto size           = new_size * sizeof(elem_ptr) + // storage for ptrs
        buffer_size + // storage for existing elems w initial alignment
        new_object_size + // storage for the new elem
        (num_of_new_obj * avg_obj_size); // estimated storage for new elems
    return std::make_pair(size, max_alignment);
}

template <class I, class A, class C>
inline auto vector<I, A, C>::occupied_storage(elem_ptr_const_pointer p) const noexcept -> size_type
{
    return ((p->size() + _align_max - 1) / _align_max) * _align_max;
}

template <class I, class A, class C>
template <class descendant_type>
inline auto vector<I, A, C>::insert(const_iterator position, descendant_type&& val)
    -> std::enable_if_t<std::is_base_of<interface_type, std::decay_t<descendant_type>>::value,
        iterator>
{
    if (position == end()) {
        push_back(std::forward<descendant_type>(val));
        return std::prev(end());
    }
    using TT         = std::decay_t<descendant_type>;
    constexpr auto s = sizeof(TT);
    constexpr auto a = alignof(TT);
    //////////////////////////////////////////
    const auto new_index = std::distance(cbegin(), position);
    const auto new_size  = size() + 1;
    const auto sizes     = calculate_storage_size(new_size, s, a);
    auto       from      = std::next(begin_elem(), new_index);

    vector v(allocator_traits::select_on_container_copy_construction(base().get_allocator_ref()));
    v.init_layout(sizes.first, new_size, sizes.second);
    v.set_ptrs(poly_uninitialized_copy(v.base(), v.begin_elem(), begin_elem(), from,
        std::next(cbegin_elem(), new_size), v.max_align()));
    v.push_back(std::forward<descendant_type>(val));

    auto dst_storage = v.free_storage();
    auto dst         = v.end_elem();

    for (; from != end_elem(); ++from, ++dst) {
        *dst           = *from;
        dst->ptr.first = next_aligned_storage(dst_storage, v.max_align());
        dst->ptr.second
            = from->policy().clone(v.base().get_allocator_ref(), from->ptr.second, dst->ptr.first);
        dst_storage  = static_cast<pointer>(dst->ptr.first) + dst->size();
        v._free_elem = dst;
    }
    v._free_elem = dst;
    this->swap(v);
    return std::next(begin(), new_index);
}

} // namespace poly
