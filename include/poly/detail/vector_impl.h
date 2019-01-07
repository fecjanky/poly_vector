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

#include <poly/detail/vector_config.h>

#include <memory>
#include <type_traits>

namespace poly {
namespace vector_impl {

#if defined(POLY_VECTOR_HAS_CXX_DISJUNCTION)
    template <typename... T> using or_type_t = typename std::disjunction<T...>::type;
#else
    template <typename... Ts> struct OrType;

    template <typename... Ts> struct OrType<::std::true_type, Ts...> {
        static constexpr bool value = true;
        using type                  = ::std::true_type;
    };

    template <typename... Ts> struct OrType<::std::false_type, Ts...> {
        static constexpr bool value = OrType<Ts...>::value;
        using type                  = typename OrType<Ts...>::type;
    };

    template <> struct OrType<> {
        static constexpr bool value = false;
        using type                  = ::std::false_type;
    };

    template <typename... Ts> using or_type_t = typename OrType<Ts...>::type;

#endif

#if defined(POLY_VECTOR_HAS_CXX_ALLOCATOR_ALWAYS_EQUAL)
    template <typename A>
    using allocator_is_always_equal_t = typename std::allocator_traits<A>::is_always_equal;
#else
    template <typename A, bool b> struct select_always_eq_trait {
        using type = typename A::is_always_equal;
    };

    template <typename A> struct select_always_eq_trait<A, false> {
        using type = typename std::is_empty<A>::type;
    };

    // until c++17...
    template <typename A> struct allocator_is_always_equal {
    private:
        template <typename AA>
        static std::true_type  has_always_equal_test(const AA&, typename AA::is_always_equal*);
        static std::false_type has_always_equal_test(...);

    public:
        static constexpr bool has_always_equal = std::is_same<std::true_type,
            decltype(has_always_equal_test(std::declval<A>(), nullptr))>::value;

        using type                  = typename select_always_eq_trait<A, has_always_equal>::type;
        static constexpr bool value = type::value;
    };

    template <typename A>
    using allocator_is_always_equal_t = typename allocator_is_always_equal<A>::type;
#endif
} // namespace vector_impl
} // namespace poly
