#include "catch_ext.hpp"
#include <algorithm>
#include <catch.hpp>
#include <cstring>
#include <iostream>
#include <vector>

#include <poly_vector.h>
#include <test_poly_vector.h>

using namespace poly::poly_vector_impl;

template <bool B> struct s_has_aeq {
    using is_always_equal = std::integral_constant<bool, B>;
};

struct no_aeq_empty {
};
struct no_aeq {
    int a;
};

static_assert(
    allocator_is_always_equal<s_has_aeq<true>>::has_always_equal, "should have always equal");

static_assert(
    allocator_is_always_equal<s_has_aeq<true>>::value, "should have always equal as true");

static_assert(
    allocator_is_always_equal<s_has_aeq<false>>::has_always_equal, "should have always equal");

static_assert(
    !allocator_is_always_equal<s_has_aeq<false>>::value, "should have always equal as false");

static_assert(
    !allocator_is_always_equal<no_aeq_empty>::has_always_equal, "should not have always equal");

static_assert(allocator_is_always_equal<no_aeq_empty>::value,
    "should have is always equal as true because empty");

static_assert(!allocator_is_always_equal<no_aeq>::has_always_equal, "should not have always equal");

static_assert(!allocator_is_always_equal<no_aeq>::value,
    "should have always equal as false because not empty");

static_assert(allocator_is_always_equal<custom::Allocator<int>>::has_always_equal,
    "should have always equal");

static_assert(
    !allocator_is_always_equal<custom::Allocator<int>>::value, "should have always equal as true");

using traits     = std::allocator_traits<custom::Allocator<int>>;
using std_traits = std::allocator_traits<std::allocator<int>>;

static_assert(!traits::propagate_on_container_move_assignment::value, "should have prop as false");

traits::rebind_alloc<uint8_t> a;
