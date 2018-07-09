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
#ifndef POLY_VECTOR_H_
#define POLY_VECTOR_H_

#include <memory>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <tuple>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <cmath>
#include <type_traits>
#include <iterator>
#include <numeric>


namespace estd
{
    namespace poly_vector_impl
    {
        template<typename... Ts>
        struct OrType;

        template<typename... Ts>
        struct OrType<::std::true_type, Ts...> {
            static constexpr bool value = true;
            using type = ::std::true_type;
        };

        template<typename... Ts>
        struct OrType<::std::false_type, Ts...> {
            static constexpr bool value = OrType<Ts...>::value;
            using type = typename OrType<Ts...>::type;
        };

        template<>
        struct OrType<> {
            static constexpr bool value = false;
            using type = ::std::false_type;
        };

        template<typename... Ts>
        using or_type_t = typename OrType<Ts...>::type;

        template<typename T>
        struct TD;

        template<typename A, bool b>
        struct select_always_eq_trait {
            using type = typename A::is_always_equal;
        };

        template<typename A>
        struct select_always_eq_trait<A,false> {
            using type = typename std::is_empty<A>::type;
        };

        // until c++17...
        template<typename A>
        struct allocator_is_always_equal {
        private:
            template<typename AA>
            static std::true_type has_always_equal_test(const AA&,typename AA::is_always_equal*);
            static std::false_type has_always_equal_test(...);
            static constexpr bool has_always_equal =
                    std::is_same<
                            std::true_type,
                            decltype(has_always_equal_test(std::declval<A>(),nullptr))
                    >::value;
        public:
            using type = typename select_always_eq_trait<A, has_always_equal>::type;
            static constexpr bool value = type::value;
        };


        template<typename A>
        using allocator_is_always_equal_t =
        typename allocator_is_always_equal<A>::type;


        template<class TT>
        struct has_noexcept_movable{
            template<class U>
            static std::true_type test(U&&,typename std::decay_t<U>::noexcept_movable *);
            static std::false_type test(...);
            static constexpr bool value =
                    std::is_same<std::true_type,decltype(test(std::declval<TT>(),nullptr))>::value;
        };

        template<class TT,bool HasType>
        struct is_noexcept_movable{
            using type = typename TT::noexcept_movable;
        };
        template<class TT>
        struct is_noexcept_movable<TT,false>{
            using type = std::false_type;
        };

        template<class TT>
        using is_noexcept_movable_t = typename is_noexcept_movable<TT,has_noexcept_movable<TT>::value>::type;

        template<class T,class IF,class A>
        struct is_cloning_policy_impl{
            using void_ptr = typename std::allocator_traits<A>::void_pointer;
            using pointer = typename std::allocator_traits<A>::pointer;

            template<class TT>
            struct has_clone_impl{
                template<class U>
                static decltype(std::declval<U>()
                        .clone(std::declval<A>(),std::declval<pointer>(),std::declval<void_ptr>()))* test(U&&);

                static std::false_type test(...);

                static constexpr bool value = std::is_same<decltype(test(std::declval<TT>())),pointer*>::value;
            };
            template<class TT>
            struct has_move_impl{
                template<class U>
                static decltype(std::declval<U>()
                        .move(std::declval<A>(),std::declval<pointer>(),std::declval<void_ptr>()))* test(U&&);
                static std::false_type test(...);
                static constexpr bool value = std::is_same<pointer*,decltype(test(std::declval<TT>()))>::value;
            };

            using has_move_t = std::integral_constant<bool,has_move_impl<T>::value>;
            using has_clone_t = std::integral_constant<bool,has_clone_impl<T>::value>;
            // TODO(fecjanky): provide traits defaults for Allocator type, pointer and void_pointer types
            static constexpr bool value =
                    std::is_same<IF,typename std::pointer_traits<pointer>::element_type>::value &&
                    std::is_nothrow_constructible<T>::value &&
                    std::is_nothrow_copy_constructible<T>::value &&
                    std::is_nothrow_copy_assignable<T>::value &&
                    (has_clone_t::value || (has_move_t::value && is_noexcept_movable_t<T>::value));
        };

        template<class Allocator>
        struct allocator_base : private Allocator
        {

            using allocator_traits = std::allocator_traits<Allocator>;
            using value_type = typename allocator_traits::value_type;
            using propagate_on_container_copy_assignment = typename allocator_traits::propagate_on_container_copy_assignment;
            using propagate_on_container_swap = typename allocator_traits::propagate_on_container_swap;
            using propagate_on_container_move_assignment = typename allocator_traits::propagate_on_container_move_assignment;
            using allocator_is_always_equal = poly_vector_impl::allocator_is_always_equal_t<Allocator>;
            static_assert(std::is_same<value_type, uint8_t>::value, "requires a byte allocator");
            using allocator_type = Allocator;
            using void_pointer = typename allocator_traits::void_pointer;
            using const_void_pointer = typename allocator_traits::const_void_pointer;
            using difference_type = typename  allocator_traits::difference_type;
            using pointer = typename allocator_traits::pointer ;
            using const_pointer = typename allocator_traits::const_pointer;
            //////////////////////////////////////////////
            explicit allocator_base(const Allocator& a = Allocator()) : Allocator(a), _storage{}, _end_storage{} {}

            explicit allocator_base(size_t n, const Allocator& a = Allocator()) : Allocator(a), _storage{}, _end_storage{}
            {
                auto s = get_allocator_ref().allocate(n);
                _storage = s;
                _end_storage = s + n;
            }

            allocator_base(const allocator_base& a) :
                    allocator_base(allocator_traits::select_on_container_copy_construction(a.get_allocator_ref()))
            {
                if (a.size())allocate(a.size());
            }

            allocator_base(allocator_base&& a) noexcept :
                    Allocator(std::move(a.get_allocator_ref())),
                    _storage{ a._storage }, _end_storage{ a._end_storage }
            {
                a._storage = a._end_storage = nullptr;
            }

            allocator_base& operator=(const allocator_base& a)
            {
                return copy_assign_impl(a, propagate_on_container_copy_assignment{});
            }

            allocator_base& operator=(allocator_base&& a)
            noexcept(propagate_on_container_move_assignment::value || allocator_is_always_equal::value)
            {
                return move_assign_impl(std::move(a), propagate_on_container_move_assignment{}, allocator_is_always_equal{});
            }

            allocator_base& move_assign_impl(allocator_base&& a, std::true_type, ...) noexcept
            {
                using std::swap;
                tidy();
                if (propagate_on_container_move_assignment::value) {
                    // must not throw
                    get_allocator_ref() = std::move(a.get_allocator_ref());
                }
                swap(a._storage, _storage);
                swap(a._end_storage, _end_storage);
                return *this;
            }

            allocator_base& move_assign_impl(allocator_base&& a, std::false_type, std::true_type) noexcept
            {
                return move_assign_impl(std::move(a), std::true_type{});
            }

            allocator_base& move_assign_impl(allocator_base&& a, std::false_type, std::false_type)
            {
                if (get_allocator_ref() != a.get_allocator_ref())
                    return copy_assign_impl(a, std::false_type{});
                else
                    return move_assign_impl(std::move(a), std::true_type{});
            }

            allocator_base& copy_assign_impl(const allocator_base& a, std::true_type)
            {
                // Note: allocator copy assignment must not throw
                pointer s = nullptr;
                if (a.size())
                    s = a.get_allocator_ref().allocate(a.size());
                tidy();
                get_allocator_ref() = a.get_allocator_ref();
                _storage = s;
                _end_storage = s + a.size();
                return *this;
            }

            allocator_base& copy_assign_impl(const allocator_base& a, std::false_type)
            {
                pointer s = nullptr;
                if (a.size())
                    s = get_allocator_ref().allocate(a.size());
                tidy();
                _storage = s;
                _end_storage = s + a.size();
                return *this;
            }

            void swap(allocator_base& x) noexcept
            {
                using std::swap;
                if (propagate_on_container_swap::value) {
                    swap(get_allocator_ref(), x.get_allocator_ref());
                }
                else if (!allocator_is_always_equal::value &&
                         get_allocator_ref() != x.get_allocator_ref()) {
                    // Undefined behavior
                    assert(0);
                }
                swap(_storage, x._storage);
                swap(_end_storage, x._end_storage);
            }
            void allocate(size_t n)
            {
                auto s = get_allocator_ref().allocate(n);
                tidy();
                _storage = s;
                _end_storage = s + n;
            }

            allocator_type& get_allocator_ref()noexcept
            {
                return *this;
            }
            const allocator_type& get_allocator_ref()const noexcept
            {
                return *this;
            }

            void tidy() noexcept
            {
                get_allocator_ref().deallocate(static_cast<pointer>(_storage), size());
                _storage = _end_storage = nullptr;
            }

            ~allocator_base()
            {
                tidy();
            }

            difference_type size()const noexcept
            {
                return static_cast<pointer>(_end_storage) -
                       static_cast<pointer>(_storage);
            }

            void_pointer storage()noexcept
            {
                return _storage;
            }
            void_pointer end_storage()noexcept
            {
                return _end_storage;
            }
            const_void_pointer storage()const noexcept
            {
                return _storage;
            }
            const_void_pointer end_storage()const noexcept
            {
                return _end_storage;
            }
            template<typename PointerType>
            void destroy(PointerType obj) const{
                using T = std::decay_t<decltype(*obj)>;
                using traits = typename allocator_traits::template rebind_traits <T>;
                static_assert(std::is_same<PointerType, typename traits::pointer>::value,"Invalid pointer type");
                typename traits::allocator_type a(get_allocator_ref());
                traits::destroy(a,obj);
            }
            template<typename PointerType,typename... Args>
            PointerType construct(PointerType storage,Args&&... args) const{
                using T = std::decay_t<decltype(*storage)>;
                using traits = typename allocator_traits::template rebind_traits <T>;
                static_assert(std::is_same<PointerType, typename traits::pointer>::value,"Invalid pointer type");
                typename traits::allocator_type a(get_allocator_ref());
                traits::construct(a,storage,std::forward<Args>(args)...);
                return storage;
            }
            ////////////////////////////////
            void_pointer _storage;
            void_pointer _end_storage;
        };

        template<class Policy,class Interface,class Allocator>
        using is_cloning_policy =
            std::integral_constant<bool,poly_vector_impl::is_cloning_policy_impl<Policy,Interface,Allocator>::value>;

        template<class Policy,class Interface,class Allocator>
        struct cloning_policy_traits{
            static_assert(is_cloning_policy<Policy,Interface,Allocator>::value,"Policy is not a cloning policy");
            using noexcept_movable = poly_vector_impl::is_noexcept_movable_t<Policy>;
            using pointer = typename Policy::pointer;
            using void_pointer = typename Policy::void_pointer;
            using allocator_type = typename Policy::allocator_type;
            static pointer move(const Policy& p, const allocator_type& a, pointer obj, void_pointer dest)
                noexcept(noexcept_movable::value)
            {
                using policy_impl = ::estd::poly_vector_impl::is_cloning_policy_impl<Policy,Interface,Allocator>;
                return move_impl(p,a,obj,dest,typename policy_impl::has_move_t{});
            }
        private:
            static pointer move_impl(const Policy& p, const allocator_type& a, pointer obj, void_pointer dest, std::true_type)
                noexcept(noexcept_movable::value)
            {
                return p.move(a,obj,dest);
            }
            static pointer move_impl(const Policy& p, const allocator_type& a, pointer obj, void_pointer dest,std::false_type)
            {
                return p.clone(a,obj,dest);
            }

        };

    }  //namespace poly_vector_impl


    template<
    class Interface,
    class Allocator = std::allocator<Interface>, 
    typename NoExceptmovable = std::true_type
    >struct delegate_cloning_policy;

    template<
        class IF,
        class Allocator = std::allocator<IF>,
        /// implicit noexcept_movability when using defaults of delegate cloning policy
        class CloningPolicy = delegate_cloning_policy<IF, Allocator>
    >class poly_vector;
    
    template<class Allocator,class CloningPolicy>
    struct poly_vector_elem_ptr : private CloningPolicy
    {
        using IF = typename  std::allocator_traits<Allocator>::value_type;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using size_func_t = std::pair<size_t, size_t>();
        using policy_t = CloningPolicy;
        poly_vector_elem_ptr() : ptr{}, sf{} {}

        template<typename T, typename = std::enable_if_t< std::is_base_of< IF, std::decay_t<T> >::value > >
        explicit poly_vector_elem_ptr(T&& t, void_pointer s = nullptr, pointer i = nullptr) noexcept :
            CloningPolicy(std::forward<T>(t)), ptr{ s, i }, sf{ &poly_vector_elem_ptr::size_func<std::decay_t<T>>}
        {}
        poly_vector_elem_ptr(const poly_vector_elem_ptr& other) noexcept :
            CloningPolicy(other.policy()), ptr{ other.ptr }, sf{ other.sf }
        {}
        poly_vector_elem_ptr& operator=(const poly_vector_elem_ptr & rhs) noexcept
        {
            policy() = rhs.policy();
            ptr = rhs.ptr;
            sf = rhs.sf;
            return *this;
        }
        void swap(poly_vector_elem_ptr& rhs) noexcept
        {
            using std::swap;
            swap(policy(), rhs.policy());
            swap(ptr, rhs.ptr);
            swap(sf, rhs.sf);
        }

        policy_t& policy()noexcept
        {
            return *this;
        }
        const policy_t& policy()const noexcept
        {
            return *this;
        }
        size_t size()const noexcept
        {
            return sf().first;
        }
        size_t align() const noexcept
        {
            return sf().second;
        }
        template<typename T>
        static std::pair<size_t, size_t> size_func()
        {
            return std::make_pair(sizeof(T), alignof(T));
        }
        ~poly_vector_elem_ptr()
        {
            ptr.first = ptr.second = nullptr;
            sf = nullptr;
        }

        std::pair<void_pointer, pointer> ptr;
    private:
        size_func_t* sf;
    };

    template<class A, class C>
    void swap(poly_vector_elem_ptr<A, C>& lhs, poly_vector_elem_ptr<A, C>& rhs) noexcept {
        lhs.swap(rhs);
    }

    template<class IF,class Allocator = std::allocator<IF> >
    struct virtual_cloning_policy
    {
        static constexpr bool nem = noexcept(std::declval<IF*>()->move(std::declval<Allocator>(),std::declval<void*>()));
        using noexcept_movable = std::integral_constant<bool, nem>;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using allocator_type = Allocator;

        virtual_cloning_policy() = default;
        template<typename T>
        virtual_cloning_policy(T&& t) {};
        pointer clone(const Allocator& a,pointer obj, void_pointer dest) const
        {
            return obj->clone(a,dest);
        }
        pointer move(const Allocator& a,pointer obj, void_pointer dest) const noexcept(nem)
        {
            return obj->move(a,dest);
        }
    };

    struct no_cloning_exception : public std::exception
    {
        const char* what() const noexcept override
        {
            return "cloning attempt with no_cloning_policy";
        }
    };

    template<class IF,class Allocator = std::allocator<IF> >
    struct no_cloning_policy
    {
        using noexcept_movable = std::false_type;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using allocator_type = Allocator;
        no_cloning_policy() = default;
        template<typename T>
        no_cloning_policy(T&& t) {};
        pointer clone(const Allocator& a,pointer obj, void_pointer dest) const
        {
            throw no_cloning_exception{};
        }
    };

    template<class Interface,class Allocator, typename NoExceptmovable>
    struct delegate_cloning_policy
    {
        enum Operation {
            Clone,
            Move
        };

        typedef Interface* (*clone_func_t)(const Allocator& a,Interface* obj, void* dest, Operation);
        using noexcept_movable = NoExceptmovable;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using allocator_type = Allocator;

        delegate_cloning_policy() noexcept :cf{} {}

        template <
            typename T,
            typename = std::enable_if_t<!std::is_same<delegate_cloning_policy, std::decay_t<T>>::value >
        >
            explicit delegate_cloning_policy(T&& t) noexcept :
        cf(&delegate_cloning_policy::clone_func< std::decay_t<T> >)
        {
            // this ensures, that if policy requires noexcept move construction that a given T type also has it
            static_assert(!noexcept_movable::value || std::is_nothrow_move_constructible<std::decay_t<T>>::value,
                "delegate cloning policy requires noexcept move constructor");
        };

        delegate_cloning_policy(const delegate_cloning_policy & other) noexcept : cf{ other.cf } {}

        template<class T>
        static pointer clone_func(const Allocator& a,pointer obj, void_pointer dest, Operation op)
        {
            using traits = typename std::allocator_traits<Allocator>::template rebind_traits <T>;
            using obj_pointer = typename traits::pointer;
            using obj_const_pointer = typename traits::const_pointer;

            typename traits::allocator_type alloc(a);
            if (op == Clone) {
                traits::construct(alloc, static_cast<obj_pointer>(dest), *static_cast<obj_const_pointer>(obj));
            } else {
                traits::construct(alloc, static_cast<obj_pointer>(dest), std::move(*static_cast<obj_pointer>(obj)));
            }
            return static_cast<obj_pointer>(dest);
        }

        pointer clone(const Allocator& a,pointer obj, void_pointer dest) const
        {
            return cf(a,obj, dest, Clone);
        }

        pointer move(const Allocator& a,pointer obj, void_pointer dest) const noexcept(noexcept_movable::value)
        {
            return cf(a,obj, dest, Move);
        }
        /////////////////////////
    private:
        clone_func_t cf;
    };


    template<class IF,class Allocator,class CP>
    class poly_vector_iterator : public std::iterator<std::random_access_iterator_tag,IF>
    {
    public:
        using interface_type = IF;
        using allocator_type = Allocator;
        using cloning_policy = CP;
        using p_elem = poly_vector_elem_ptr<Allocator, CP>;
        using elem = std::conditional_t<std::is_const<IF>::value,std::add_const_t<p_elem>,p_elem>;
        using void_ptr = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer ;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using reference = decltype(*std::declval<pointer>());
        using const_reference = decltype(*std::declval<const_pointer>());
        using elem_ptr = typename std::pointer_traits<pointer>::template rebind <elem>;
        using difference_type = typename std::pointer_traits<elem_ptr>::difference_type;
        using value_type = typename std::pointer_traits<elem_ptr>::element_type;

        poly_vector_iterator() : curr{} {}
        explicit poly_vector_iterator(elem_ptr p) : curr{ p } {}
        
        template<class T , 
            typename = std::enable_if_t<
                 std::is_same<std::remove_const_t<IF>,T>::value &&
                !std::is_same<poly_vector_iterator, poly_vector_iterator<T, Allocator, CP>>::value
            >
        >
        poly_vector_iterator(const poly_vector_iterator<T, Allocator, CP>& other) : curr{other.get()} {}
        
        poly_vector_iterator(const poly_vector_iterator&) = default;
        poly_vector_iterator(poly_vector_iterator&&) = default;
        poly_vector_iterator& operator=(const poly_vector_iterator&) = default;
        poly_vector_iterator& operator=(poly_vector_iterator&&) = default;

        ~poly_vector_iterator() = default;

        void swap(poly_vector_iterator& rhs)
        {
            using std::swap;
            swap(curr, rhs.curr);
        }

        pointer operator->() noexcept
        {
            return curr->ptr.second;
        }
        reference operator*()noexcept
        {
            return *curr->ptr.second;
        }
        const_pointer operator->() const noexcept
        {
            return curr->ptr.second;
        }
        const_reference operator*()const noexcept
        {
            return *curr->ptr.second;
        }

        poly_vector_iterator& operator++()noexcept
        {
            ++curr;
            return *this;
        }
        poly_vector_iterator operator++(int)noexcept
        {
            poly_vector_iterator i(*this);
            ++curr;
            return i;
        }
        poly_vector_iterator& operator--()noexcept
        {
            --curr;
            return *this;
        }
        poly_vector_iterator operator--(int)noexcept
        {
            poly_vector_iterator i(*this);
            --curr;
            return i;
        }
        poly_vector_iterator& operator+=(difference_type n)
        {
            curr += n;
            return *this;
        }
        poly_vector_iterator& operator-=(difference_type n)
        {
            curr -= n;
            return *this;
        }
        difference_type operator-(poly_vector_iterator rhs)const
        {
            return curr - rhs.curr;
        }
        reference operator[](difference_type n)
        {
            return *(curr[n]->ptr.second);
        }
        const_reference operator[](difference_type n) const
        {
            return *(curr[n]->ptr.second);
        }
        bool operator==(const poly_vector_iterator& rhs)const noexcept
        {
            return curr == rhs.curr;
        }
        bool operator!=(const poly_vector_iterator& rhs)const noexcept
        {
            return curr != rhs.curr;
        }
        bool operator<(const poly_vector_iterator& rhs)const noexcept
        {
            return curr < rhs.curr;
        }
        bool operator>(const poly_vector_iterator& rhs)const noexcept
        {
            return curr > rhs.curr;
        }
        bool operator<=(const poly_vector_iterator& rhs)const noexcept
        {
            return curr <= rhs.curr;
        }
        bool operator>=(const poly_vector_iterator& rhs)const noexcept
        {
            return curr >= rhs.curr;
        }

        elem_ptr get() const noexcept
        {
            return curr;
        }
    private:
        elem_ptr curr;
    };

    template<class IF, class Allocator, class CP>
    void swap(poly_vector_iterator<IF, Allocator, CP>& lhs, poly_vector_iterator<IF, Allocator, CP>& rhs)
    {
        lhs.swap(rhs);
    }
    template<class IF, class Allocator, class CP>
    poly_vector_iterator<IF, Allocator, CP> operator+(poly_vector_iterator<IF, Allocator, CP> a, typename poly_vector_iterator<IF, Allocator, CP>::difference_type n)
    {
        auto temp = a;
        return a += n;
    }
    template<class IF, class Allocator, class CP>
    poly_vector_iterator<IF, Allocator, CP> operator+(typename poly_vector_iterator<IF, Allocator, CP>::difference_type n, poly_vector_iterator<IF, Allocator, CP> a )
    {
        auto temp = a;
        return a += n;
    }
    template<class IF, class Allocator, class CP>
    poly_vector_iterator<IF, Allocator, CP> operator-(poly_vector_iterator<IF, Allocator, CP> i, typename poly_vector_iterator<IF, Allocator, CP>::difference_type n)
    {
        auto temp = i;
        return i -= n;
    }

    template<class IF, class Allocator, class CloningPolicy>class poly_vector :
        private poly_vector_impl::allocator_base<
        typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>
        >
    {
    public:
        ///////////////////////////////////////////////
        // Member types
        ///////////////////////////////////////////////
        using allocator_type                    = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;
        using interface_allocator_type          = typename std::allocator_traits<Allocator>::template rebind_alloc<IF>;
        using my_base                           = poly_vector_impl::allocator_base<allocator_type>;
        using interface_type                    = std::decay_t<IF>;
        using interface_pointer                 = typename interface_allocator_type ::pointer;
        using const_interface_pointer           = typename interface_allocator_type ::const_pointer ;
        using interface_reference               = typename interface_allocator_type::reference ;
        using const_interface_reference         = typename interface_allocator_type::const_reference;
        using allocator_traits                  = std::allocator_traits<allocator_type>;
        using pointer                           = typename my_base::pointer;
        using const_pointer                     = typename my_base::const_pointer;
        using void_pointer                      = typename my_base::void_pointer;
        using const_void_pointer                = typename my_base::const_void_pointer;
        using size_type                         = std::size_t;
        using iterator                          = poly_vector_iterator<interface_type ,interface_allocator_type, CloningPolicy>;
        using const_iterator                    = poly_vector_iterator<const interface_type,interface_allocator_type,CloningPolicy>;
        using reverse_iterator                  = std::reverse_iterator<iterator>;
        using const_reverse_iterator            = std::reverse_iterator<const_iterator>;
        using move_is_noexcept_t                = std::is_nothrow_move_assignable<my_base>;
        using cloning_policy_traits             = poly_vector_impl::cloning_policy_traits<CloningPolicy,interface_type,interface_allocator_type>;
        using interface_type_noexcept_movable   = typename cloning_policy_traits::noexcept_movable;

        static_assert(std::is_same<IF,interface_type>::value,
                      "Interface type must be a non-cv qualified user defined type");
        static_assert(std::is_polymorphic<interface_type>::value, "interface_type is not polymorphic");
        static_assert(poly_vector_impl::is_cloning_policy<CloningPolicy,interface_type,Allocator>::value,"invalid cloning policy type");
        static constexpr auto default_avg_size = 4 * sizeof(void*);
        static constexpr auto default_alignement = alignof(interface_reference);
        ///////////////////////////////////////////////
        // Ctors,Dtors & assignment
        ///////////////////////////////////////////////
                    poly_vector();
        explicit    poly_vector(const allocator_type& alloc) ;
                    poly_vector(const poly_vector& other);
                    poly_vector(poly_vector&& other);
                    ~poly_vector();

        poly_vector&        operator=(const poly_vector& rhs);
        poly_vector&        operator=(poly_vector&& rhs) noexcept(move_is_noexcept_t::value);
        ///////////////////////////////////////////////
        // Modifiers
        ///////////////////////////////////////////////
        template<typename T>
        std::enable_if_t<std::is_base_of<interface_type, std::decay_t<T>>::value,
                void>       push_back(T&& obj);
        void                pop_back()              noexcept;
        void                clear()                 noexcept;
        void                swap(poly_vector& x)    noexcept;

        // TODO: insert, erase,emplace, emplace_back, assign?
        template<class descendant_type>
        std::enable_if_t<std::is_base_of<interface_type, std::decay_t<descendant_type>>::value, iterator>
            insert( const_iterator position, descendant_type&& val );
        //template <class InputIterator>
        //iterator insert (const_iterator position, InputIterator first, InputIterator last);
        //iterator insert (const_iterator position, polyvectoriterator first, polyvectoriterator last);
        iterator                    erase(const_iterator position);
        iterator                    erase(const_iterator first, const_iterator last);
        ///////////////////////////////////////////////
        // Iterators
        ///////////////////////////////////////////////
        iterator                    begin()     noexcept;
        iterator                    end()       noexcept;
        const_iterator              begin()     const   noexcept;
        const_iterator              end()       const   noexcept;
        reverse_iterator            rbegin()    noexcept;
        reverse_iterator            rend()      noexcept;
        const_iterator              rbegin()    const   noexcept;
        const_iterator              rend()      const   noexcept;
        ///////////////////////////////////////////////
        // Capacity
        ///////////////////////////////////////////////
        size_t                      size()          const   noexcept;
        std::pair<size_t, size_t>   sizes()         const   noexcept;
        size_type                   capacity()      const   noexcept;
        std::pair<size_t, size_t>   capacities()    const   noexcept;
        bool                        empty()         const   noexcept;
        size_type                   max_size()      const   noexcept;
        size_type                   max_align()     const   noexcept;
        void                        reserve(size_type n, size_type avg_size, size_type max_align = alignof(std::max_align_t));
        void                        reserve(size_type n);
        void                        reserve(std::pair<size_t, size_t> s);
        // TODO(fecjanky): void shrink_to_fit();
        ///////////////////////////////////////////////
        // Element access
        ///////////////////////////////////////////////
        interface_reference         operator[](size_t n)    noexcept;
        const_interface_reference   operator[](size_t n)    const   noexcept;
        interface_reference         at(size_t n);
        const_interface_reference   at(size_t n)            const;
        interface_reference         front()                 noexcept;
        const_interface_reference   front()                 const   noexcept;
        interface_reference         back()                  noexcept;
        const_interface_reference   back()                  const   noexcept;

        std::pair<void_pointer, void_pointer>               data() noexcept;
        std::pair<const_void_pointer,const_void_pointer>    data() const    noexcept;
        ////////////////////////////
        // Misc.
        ////////////////////////////
        allocator_type  get_allocator()     const   noexcept;
    private:
        using elem_ptr                  = poly_vector_elem_ptr<typename allocator_traits::template rebind_alloc<interface_type>, CloningPolicy>;
        using elem_ptr_pointer          = typename allocator_traits::template rebind_traits<elem_ptr>::pointer;
        using elem_ptr_const_pointer    = typename allocator_traits::template rebind_traits<elem_ptr>::const_pointer;
        using poly_copy_descr           = std::tuple<elem_ptr_pointer , elem_ptr_pointer, void_pointer >;

        ////////////////////////
        /// Storage management helpers
        ////////////////////////
        static void_pointer         next_aligned_storage(void_pointer p, size_t align)  noexcept;
        static size_t               storage_size(const_void_pointer b, const_void_pointer e)        noexcept;
        static size_t               max_alignment(elem_ptr_const_pointer begin, elem_ptr_const_pointer end)     noexcept;
        static size_t               max_alignment(elem_ptr_const_pointer begin1, elem_ptr_const_pointer end1, elem_ptr_const_pointer begin2, elem_ptr_const_pointer end2)   noexcept;
        static size_t               occupied_storage_size(elem_ptr_const_pointer p) noexcept;
        size_t                      max_alignment()     const   noexcept;
        size_t                      max_alignment(size_t new_alignment)     const   noexcept;
        std::pair<size_t,size_t>    calculate_storage_size(size_t new_size,size_t new_elem_size, size_t new_elem_alignment) const noexcept;
        size_type                   occupied_storage(elem_ptr_const_pointer p)  const   noexcept;

        template<typename CopyOrMove>
        void increase_storage(
            size_t desired_size,
            size_t curr_elem_size,
            size_t align,
            CopyOrMove);

        void obtain_storage(
            my_base&& a,
            size_t n,
            size_t max_align,
            std::true_type);

        void obtain_storage(
            my_base&& a,
            size_t n,
            size_t max_align,
            std::false_type) noexcept;

        my_base&                base()  noexcept;
        const my_base&          base()  const   noexcept;
        poly_copy_descr         poly_uninitialized_copy(const allocator_type& a,elem_ptr_pointer dst, size_t n) const;
        poly_copy_descr         poly_uninitialized_move(const allocator_type& a,elem_ptr_pointer dst, size_t n) const noexcept;
        poly_vector&            copy_assign_impl(const poly_vector& rhs);
        poly_vector&            move_assign_impl(poly_vector&& rhs, std::true_type) noexcept;
        poly_vector&            move_assign_impl(poly_vector&& rhs, std::false_type);
        void                    tidy()  noexcept;
        void                    destroy_elem(elem_ptr_pointer p)    noexcept;
        std::pair<void_pointer,void_pointer>
                                destroy_range(elem_ptr_pointer first, elem_ptr_pointer last)    noexcept;
        iterator                erase_internal_range(elem_ptr_pointer first, elem_ptr_pointer last);
        void                    clear_till_end(elem_ptr_pointer first)    noexcept;
        void                    init_ptrs(size_t n)     noexcept;
        void                    set_ptrs(poly_copy_descr p);
        void                    swap_ptrs(poly_vector&& rhs);
        template<class T>
        void                    push_back_new_elem(T&& obj);
        template<class T>
        void                    push_back_new_elem_w_storage_increase(T&& obj);
        void                    push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::true_type);
        void                    push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::false_type);
        bool                    can_construct_new_elem(size_t s, size_t align)  noexcept;
        void_pointer            next_aligned_storage(size_t align)  const   noexcept;
        size_t                  avg_obj_size(size_t align = 1)  const   noexcept;
        elem_ptr_pointer        begin_elem()    noexcept;
        elem_ptr_pointer        end_elem()      noexcept;
        elem_ptr_const_pointer  begin_elem()    const   noexcept;
        elem_ptr_const_pointer  end_elem()      const   noexcept;
        ////////////////////////////
        // Members
        ////////////////////////////
        elem_ptr_pointer        _free_elem;
        void_pointer            _begin_storage;
        void_pointer            _free_storage;
        size_t                  _align_max;
    };

    template<
        class IF,
        class Allocator,
        class CloningPolicy
    >
        void swap(poly_vector<IF, Allocator, CloningPolicy>& lhs, poly_vector<IF, Allocator, CloningPolicy>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
    // TODO: relational operators

    /////////////////////////
    // implementation
    ////////////////////////
    template<class I,class A,class C>
    inline poly_vector<I, A, C>::poly_vector() : _free_elem{}, _begin_storage{}, _free_storage{}, _align_max{1} {}

    template<class I,class A,class C>
    inline poly_vector<I,A,C>::poly_vector(const allocator_type& alloc) : poly_vector_impl::allocator_base<allocator_type>(alloc),
        _free_elem{}, _begin_storage{}, _free_storage{}, _align_max{1} {};

    template<class I,class A,class C>
    inline poly_vector<I,A,C>::poly_vector(const poly_vector& other) : poly_vector_impl::allocator_base<allocator_type>(other.base()),
                                            _free_elem{ begin_elem() }, _begin_storage{ begin_elem() + other.capacity() },
                                            _free_storage{ _begin_storage }, _align_max{other._align_max}
    {
        set_ptrs(other.poly_uninitialized_copy(base().get_allocator_ref(),begin_elem(), other.size()));
    }

    template<class I,class A,class C>
    inline poly_vector<I,A,C>::poly_vector(poly_vector&& other) : poly_vector_impl::allocator_base<allocator_type>(std::move(other.base())),
    _free_elem{ other._free_elem }, _begin_storage{ other._begin_storage },
        _free_storage{ other._free_storage }, _align_max{ other._align_max }
    {
        other._begin_storage = other._free_storage = other._free_elem = nullptr;
        other._align_max = default_alignement;
    }

    template<class I,class A,class C>
    inline poly_vector<I,A,C>::~poly_vector()
    {
        tidy();
    }

    template<class I,class A,class C>
    inline poly_vector<I,A,C>& poly_vector<I,A,C>::operator=(const poly_vector& rhs)
    {
        if (this != &rhs) {
            copy_assign_impl(rhs);
        }
        return *this;
    }

    template<class I,class A,class C>
    inline poly_vector<I,A,C>& poly_vector<I,A,C>::operator=(poly_vector&& rhs) noexcept(move_is_noexcept_t::value)
    {
        if (this != &rhs) {
            move_assign_impl(std::move(rhs), move_is_noexcept_t{});
        }
        return *this;
    }

    template<class I,class A,class C>
    template<typename T>
    inline auto poly_vector<I,A,C>::push_back(T&& obj) ->
        std::enable_if_t<std::is_base_of<interface_type, std::decay_t<T>>::value>
    {
        using TT = std::decay_t<T>;
        constexpr auto s = sizeof(TT);
        constexpr auto a = alignof(TT);
        if (end_elem() == _begin_storage || !can_construct_new_elem(s, a)) {
            push_back_new_elem_w_storage_increase(std::forward<T>(obj));
        }
        else {
            push_back_new_elem(std::forward<T>(obj));
        }
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::pop_back()noexcept
    {
        clear_till_end(_free_elem-1);
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::clear() noexcept
    {
        clear_till_end(begin_elem());
        _align_max = default_alignement;
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::swap(poly_vector& x) noexcept
    {
        using std::swap;
        base().swap(x.base());
        swap(_free_elem, x._free_elem);
        swap(_begin_storage, x._begin_storage);
        swap(_free_storage, x._free_storage);
        swap(_align_max, x._align_max);
    }

    template<class I, class A, class C>
    inline auto poly_vector<I, A, C>::erase(const_iterator position) -> iterator
    {
        return erase(position,position+1);
    }

    template<class I, class A, class C>
    inline auto poly_vector<I, A, C>::erase(const_iterator first, const_iterator last) -> iterator
    {
        auto eptr_first = begin_elem() + (first - begin());
        if (last == end()) {
            clear_till_end(eptr_first);
            if (empty())_align_max = default_alignement;
            return end();
        }
        else {
            auto it = erase_internal_range(eptr_first, begin_elem() + (last - begin()));
            if (empty())_align_max = default_alignement;
            return it;
        }
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::begin() noexcept -> iterator
    {
        return iterator(begin_elem());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::end()noexcept-> iterator
    {
        return iterator(end_elem());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::begin()const noexcept -> const_iterator
    {
        return const_iterator(begin_elem());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::end()const noexcept -> const_iterator
    {
        return const_iterator(end_elem());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::rbegin()noexcept -> reverse_iterator
    {
        return reverse_iterator(--end());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::rend()noexcept -> reverse_iterator
    {
        return reverse_iterator(--begin());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::rbegin()const noexcept -> const_iterator
    {
        return const_iterator(--end());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::rend()const noexcept -> const_iterator
    {
        return const_iterator(--begin());
    }

    template<class I,class A,class C>
    inline size_t poly_vector<I,A,C>::size()const noexcept
    {
        return static_cast<size_t>(_free_elem-begin_elem());
    }

    template<class I,class A,class C>
    inline std::pair<size_t, size_t> poly_vector<I,A,C>::sizes() const noexcept
    {
        return std::make_pair(size(), avg_obj_size());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::capacity() const noexcept -> size_type
    {
        return storage_size(begin_elem(), _begin_storage) / sizeof(elem_ptr);
    }

    template<class I,class A,class C>
    inline std::pair<size_t, size_t> poly_vector<I,A,C>::capacities() const noexcept
    {
        return std::make_pair(capacity(), storage_size(_begin_storage, this->_end_storage));
    }

    template<class I,class A,class C>
    inline bool poly_vector<I,A,C>::empty() const noexcept
    {
        return begin_elem() == _free_elem;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::max_size() const noexcept -> size_type
    {
        auto avg = avg_obj_size() ? avg_obj_size() : 4 * sizeof(void_pointer);
        return allocator_traits::max_size(this->get_allocator_ref()) /
               (sizeof(elem_ptr) + avg);
    }

    template<class I, class A, class C>
    inline auto poly_vector<I, A, C>::max_align() const noexcept -> size_type
    {
        return _align_max;
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::reserve(size_type n, size_type avg_size,size_type max_align)
    {
        using copy = std::conditional_t<interface_type_noexcept_movable::value,
                std::false_type,std::true_type>;
        if (n <= capacities().first && avg_size <= capacities().second && _align_max >= max_align)  return;
        if (n > max_size())throw std::length_error("poly_vector reserve size too big");
        increase_storage(n, avg_size, max_align, copy{});
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::reserve(size_type n) {
        reserve(n, default_avg_size);
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::reserve(std::pair<size_t, size_t> s)
    {
        reserve(s.first, s.second);
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::operator[](size_t n) noexcept -> interface_reference
    {
        return *begin_elem()[n].ptr.second;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::operator[](size_t n)const noexcept -> const_interface_reference
    {
        return *begin_elem()[n].ptr.second;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::at(size_t n) -> interface_reference
    {
        if (n >= size()) throw std::out_of_range{ "poly_vector out of range access" };
        return (*this)[n];
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::at(size_t n)const -> const_interface_reference
    {
        if (n >= size()) throw std::out_of_range{ "poly_vector out of range access" };
        return (*this)[n];
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::front() noexcept -> interface_reference
    {
        return (*this)[0];
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::front()const noexcept -> const_interface_reference
    {
        return (*this)[0];
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::back() noexcept -> interface_reference
    {
        return (*this)[size() - 1];
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::back()const noexcept -> const_interface_reference
    {
        return (*this)[size() - 1];
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::data() noexcept -> std::pair<void_pointer, void_pointer>
    {
        return std::make_pair(base()._storage,base()._end_storage);
    }
    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::data() const noexcept -> std::pair<const_void_pointer,const_void_pointer>
    {
        return std::make_pair(base()._storage, base()._end_storage);
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::get_allocator() const noexcept -> allocator_type
    {
        return my_base::get_allocator_ref();
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::next_aligned_storage(void_pointer p, size_t align) noexcept -> void_pointer
    {
        auto v = static_cast<pointer>(p) - static_cast<pointer>(nullptr);
        auto a = ((v + align - 1)/align)*align - v;
        return static_cast<pointer>(p) + a;
    }

    template<class I,class A,class C>
    inline size_t poly_vector<I,A,C>::storage_size(const_void_pointer b, const_void_pointer e) noexcept
    {
        using const_pointer_t = typename allocator_traits::const_pointer;
        return static_cast<size_t>(static_cast<const_pointer_t>(e)-static_cast<const_pointer_t>(b));
    }

    template<class I,class A,class C>
    template<typename CopyOrMove>
    inline void poly_vector<I,A,C>::increase_storage(size_t desired_size, size_t curr_elem_size, size_t align ,CopyOrMove)
    {
        auto sizes = calculate_storage_size(desired_size, curr_elem_size, align);
        my_base s(sizes.first, allocator_traits::select_on_container_copy_construction(base().get_allocator_ref()));
        obtain_storage(std::move(s), desired_size, sizes.second, CopyOrMove{});
    }



    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::obtain_storage(my_base&& a, size_t n, size_t max_align,
                               std::true_type)
    {
        auto ret = poly_uninitialized_copy(a.get_allocator_ref(),
                                               static_cast<elem_ptr_pointer>(a.storage()), n);
        tidy();
        base().swap(a);
        set_ptrs(ret);
        _align_max = max_align;
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::obtain_storage(my_base&& a, size_t n, size_t max_align,
                               std::false_type) noexcept
    {
        _align_max = max_align;
        auto ret = poly_uninitialized_move(a.get_allocator_ref(),
                                               static_cast<elem_ptr_pointer>(a.storage()), n);
        tidy();
        base().swap(a);
        set_ptrs(ret);
        _align_max = max_align;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::base()noexcept -> my_base&
    {
        return *this;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::base()const noexcept -> const my_base &
    {
        return *this;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::poly_uninitialized_copy(const allocator_type& a,elem_ptr_pointer dst, size_t n) const -> poly_copy_descr
    {
        const auto dst_begin = dst;
        const auto storage_begin = dst + n;
        void_pointer dst_storage = storage_begin;
        for (auto elem_dst = dst_begin; elem_dst != storage_begin; ++elem_dst)
            base().construct(elem_dst);
        try {
            for (auto elem = begin_elem(); elem != _free_elem; ++elem, ++dst) {
                *dst = *elem;
                dst->ptr.first = next_aligned_storage(dst_storage, _align_max);
                dst->ptr.second = elem->policy().clone(a,elem->ptr.second, dst->ptr.first);
                dst_storage = static_cast<pointer>(dst->ptr.first) + dst->size();
            }
            return std::make_tuple(dst, storage_begin, dst_storage);
        } catch (...) {
            while(dst-- != dst_begin)
            {
                base().destroy(dst->ptr.second);
            }
            for (auto elem_dst = dst_begin; elem_dst != storage_begin; ++elem_dst)
                base().destroy(elem_dst);
            throw;
        }
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::poly_uninitialized_move(const allocator_type& a,elem_ptr_pointer dst, size_t n) const noexcept -> poly_copy_descr
    {
        const auto dst_begin = dst;
        const auto storage_begin = dst + n;
        void_pointer dst_storage = storage_begin;
        for (auto elem_dst = dst_begin; elem_dst != storage_begin; ++elem_dst)
            base().construct(elem_dst);
        for (auto elem = begin_elem(); elem != _free_elem; ++elem, ++dst) {
            *dst = *elem;
            dst->ptr.first = next_aligned_storage(dst_storage, _align_max);
            dst->ptr.second =
                    elem->policy().move(a,elem->ptr.second, dst->ptr.first);
            dst_storage = static_cast<pointer>(dst->ptr.first) + dst->size();
        }
        return std::make_tuple(dst, storage_begin, dst_storage);
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::copy_assign_impl(const poly_vector& rhs) -> poly_vector&
    {
        tidy();
        base() = rhs.base();
        init_ptrs(rhs.capacity());
        _align_max = rhs._align_max;
        set_ptrs(rhs.poly_uninitialized_copy(base().get_allocator_ref(), begin_elem(), rhs.size()));
        return *this;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::move_assign_impl(poly_vector&& rhs, std::true_type) noexcept -> poly_vector&
    {
        using std::swap;
        base().swap(rhs.base());
        swap_ptrs(std::move(rhs));
        swap(_align_max,rhs._align_max);
        return *this;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::move_assign_impl(poly_vector&& rhs, std::false_type) -> poly_vector&
    {
        if (base().get_allocator_ref() == rhs.base().get_allocator_ref()) {
            return move_assign_impl(std::move(rhs), std::true_type{});
        }
        else
            return copy_assign_impl(rhs);
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::tidy() noexcept
    {
        clear();
        for (auto i = begin_elem(); i != begin_elem()+capacity(); ++i)base().destroy(i);
        _begin_storage = _free_storage = _free_elem = nullptr;
        my_base::tidy();
    }

    template<class I, class A, class C>
    inline void poly_vector<I, A, C>::destroy_elem(elem_ptr_pointer p) noexcept
    {
        base().destroy(p->ptr.second);
        *p = elem_ptr();
    }

    template<class I, class A, class C>
    inline auto poly_vector<I, A, C>::destroy_range(elem_ptr_pointer first, elem_ptr_pointer last) noexcept -> std::pair<void_pointer, void_pointer>
    {
        std::pair<void_pointer, void_pointer> ret{};
        if (first != last) {
            ret.first = first->ptr.first;
            ret.second = last != end_elem() ? last->ptr.first : base().end_storage();
        }
        for (; first != last; ++first) {
            destroy_elem(first);
        }
        return ret;
    }

    template<class I, class A, class C>
    inline auto poly_vector<I, A, C>::erase_internal_range(elem_ptr_pointer first, elem_ptr_pointer last) -> iterator
    {
        assert(first != last);
        assert(last != end_elem());
        using std::swap;
        auto return_iterator = first;
        auto free_range = destroy_range(first, last);
        for (; last != end_elem() && occupied_storage(last) <= storage_size(free_range.first, free_range.second); ++last, ++first) {
            try {
                auto clone = cloning_policy_traits::move(last->policy(), base().get_allocator_ref(), last->ptr.second, free_range.first);
                base().destroy(last->ptr.second);
                swap(*first, *last);
                first->ptr = std::make_pair(free_range.first,clone);
                const auto next_storage = [](void_pointer p, size_type s, size_t a) { return next_aligned_storage(static_cast<pointer>(p) + s, a); };
                free_range = std::make_pair(next_storage(free_range.first,first->size(),_align_max), next_storage(free_range.second, first->size(), _align_max));
            }
            catch (...) {
                destroy_range(last, end_elem());
                _free_elem = first;
                _free_storage = static_cast<pointer>((end_elem() - 1)->ptr.first) + (end_elem() - 1)->size();
                throw;
            }
        }
        for (; last != end_elem(); ++last, ++first) swap(*first, *last);
        _free_elem = first;
        _free_storage = static_cast<pointer>((end_elem() - 1)->ptr.first) + (end_elem() - 1)->size();
        return iterator(return_iterator);
    }

    template<class I, class A, class C>
    inline void poly_vector<I, A, C>::clear_till_end(elem_ptr_pointer first) noexcept
    {
        destroy_range(first, _free_elem);
        _free_elem = first;
        _free_storage = (_free_elem != begin_elem()) ? static_cast<pointer>(_free_elem[-1].ptr.first) + _free_elem[-1].size() : _begin_storage;
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::set_ptrs(poly_copy_descr p)
    {
        _free_elem = std::get<0>(p);
        _begin_storage = std::get<1>(p);
        _free_storage = std::get<2>(p);
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::swap_ptrs(poly_vector&& rhs)
    {
        using std::swap;
        swap(_free_elem, rhs._free_elem);
        swap(_begin_storage, rhs._begin_storage);
        swap(_free_storage, rhs._free_storage);
    }

    template<class I,class A,class C>
    template<class T>
    inline void poly_vector<I,A,C>::push_back_new_elem(T&& obj)
    {
        using TT = std::decay_t<T>;
        constexpr auto s = sizeof(TT);
        constexpr auto a = alignof(TT);
        using traits = typename  allocator_traits ::template rebind_traits <TT>;

        assert(can_construct_new_elem(s, a));
        assert(_align_max >= a);
        auto nas = next_aligned_storage(_align_max);
        *_free_elem = elem_ptr(std::forward<T>(obj),nas, base().construct(static_cast<typename traits::pointer>(nas),std::forward<T>(obj)));
        _free_storage = static_cast<pointer>(_free_elem->ptr.first) + s;
        ++_free_elem;
    }

    template<class I,class A,class C>
    template<class T>
    inline void poly_vector<I,A,C>::push_back_new_elem_w_storage_increase(T&& obj)
    {
        using TT = std::decay_t<T>;
        constexpr auto s = sizeof(TT);
        constexpr auto a = alignof(TT);
        //////////////////////////////////////////
        const auto new_capacity = std::max(capacity() * 2, size_t(1));;
        const auto sizes = calculate_storage_size(new_capacity, s, a);
        poly_vector v(allocator_traits::select_on_container_copy_construction(base().get_allocator_ref()));

        v.base().allocate(sizes.first);
        v.init_ptrs(new_capacity);
        v._align_max = sizes.second;
        push_back_new_elem_w_storage_increase_copy(v, interface_type_noexcept_movable{});
        v.push_back_new_elem(std::forward<T>(obj));
        this->swap(v);
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::true_type)
    {
        v.set_ptrs(poly_uninitialized_move(base().get_allocator_ref(), v.begin_elem(), v.capacity()));
    }

    template<class I,class A,class C>
    inline void poly_vector<I,A,C>::push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::false_type)
    {
        v.set_ptrs(poly_uninitialized_copy(base().get_allocator_ref(),v.begin_elem(), v.capacity()));
    }


    template<class I,class A,class C>
    inline bool poly_vector<I,A,C>::can_construct_new_elem(size_t s, size_t align) noexcept
    {
        if (align > _align_max) return false;
        auto free = static_cast<pointer>(next_aligned_storage(_free_storage, _align_max));
        return free + s <= this->end_storage();
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::next_aligned_storage(size_t align) const noexcept -> void_pointer
    {
        return next_aligned_storage(_free_storage, align);
    }

    template<class I,class A,class C>
    inline size_t poly_vector<I,A,C>::avg_obj_size(size_t align)const noexcept
    {
        return size() ?
               (static_cast<size_t>(storage_size(_begin_storage, next_aligned_storage(align))) + size() - 1) / size() : 0;
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::begin_elem()noexcept -> elem_ptr_pointer
    {
        return static_cast<elem_ptr_pointer>(this->storage());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::end_elem()noexcept -> elem_ptr_pointer
    {
        return static_cast<elem_ptr_pointer>(_free_elem);
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::begin_elem()const noexcept -> elem_ptr_const_pointer
    {
        return static_cast<elem_ptr_const_pointer>(this->storage());
    }

    template<class I,class A,class C>
    inline auto poly_vector<I,A,C>::end_elem()const noexcept -> elem_ptr_const_pointer
    {
        return static_cast<elem_ptr_const_pointer>(_free_elem);
    }

    template<class I, class A, class C>
    inline size_t poly_vector<I, A, C>::max_alignment() const noexcept
    {
        return max_alignment(begin_elem(), end_elem());
    }
    
    template<class I, class A, class C>
    size_t poly_vector<I, A, C>::max_alignment(size_t new_alignment) const noexcept
    {
        return std::max(new_alignment, max_alignment());
    }
    
    template<class I, class A, class C>
    inline void poly_vector<I,A,C>::init_ptrs(size_t cap) noexcept
    {
        _free_elem = static_cast<elem_ptr_pointer>(base().storage());
        _begin_storage = _free_storage = begin_elem() + cap;
    }

    template<class I, class A, class C>
    inline size_t poly_vector<I, A, C>::max_alignment(elem_ptr_const_pointer begin, elem_ptr_const_pointer end) noexcept
    {
        if (begin != end) {
            return  std::max_element(begin, end,
                [](const auto& lhs, const auto& rhs) {return lhs.align() < rhs.align(); })->align();
        }
        else
            return alignof(std::max_align_t);
    }

    template<class I, class A, class C>
    inline size_t poly_vector<I, A, C>::max_alignment(elem_ptr_const_pointer begin1, elem_ptr_const_pointer end1, elem_ptr_const_pointer begin2, elem_ptr_const_pointer end2) noexcept
    {
        return std::max(max_alignment(begin1,end1),max_alignment(begin2,end2));
    }

    template<class I, class A, class C>
    inline size_t poly_vector<I, A, C>::occupied_storage_size(elem_ptr_const_pointer p) noexcept
    {
        return size_t();
    }
    
    template<class I, class A, class C>
    inline std::pair<size_t,size_t> poly_vector<I, A, C>::calculate_storage_size(
        size_t new_size,size_t new_elem_size, size_t new_elem_alignment) const noexcept
    {
        auto max_alignment = this->max_alignment(new_elem_alignment);
        auto initial_alignment_buffer = max_alignment;
        auto new_object_size = ((new_elem_size + max_alignment - 1) / max_alignment)*max_alignment;
        auto buffer_size = std::accumulate(begin_elem(),end_elem(),initial_alignment_buffer,
            [max_alignment](size_t val,const auto& p) {
            return val + ((p.size()+ max_alignment - 1) / max_alignment)*max_alignment;
        });
        auto avg_obj_size = !empty() ? (buffer_size + new_object_size + size()) / (size() + 1) : new_object_size;
        auto num_of_new_obj = new_size >= size() ? (new_size - size()) : 0U;
        auto size = new_size * sizeof(elem_ptr) +  // storage for ptrs
            buffer_size +  // storage for existing elems w initial alignment
            new_object_size +  // storage for the new elem
            (num_of_new_obj * avg_obj_size); // estimated storage for new elems
        return std::make_pair(size, max_alignment);
    }

    template<class I, class A, class C>
    inline auto poly_vector<I, A, C>::occupied_storage(elem_ptr_const_pointer p) const noexcept -> size_type
    {
        return ((p->size() + _align_max - 1) / _align_max) * _align_max;
    }

    template<class I, class A, class C>
    template<class descendant_type>
    inline auto poly_vector<I, A, C>::insert( const_iterator position, descendant_type && val ) ->
        std::enable_if_t<std::is_base_of<interface_type, std::decay_t<descendant_type>>::value, iterator>
    {
        if (position == end()) {
            push_back( std::forward<descendant_type>( val ) );
            return std::prev( end() );
        }
        return iterator();
    }

}  // namespace estd

#endif  //POLY_VECTOR_H_
