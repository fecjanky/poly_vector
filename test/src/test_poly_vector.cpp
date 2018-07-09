#include <algorithm>
#include <catch.hpp>
#include <iostream>
#include <vector>

#include "catch_ext.hpp"

#include "poly_vector.h"
#include "test_poly_vector.h"

std::atomic<size_t> Interface::last_id{ 0 };

TEST_CASE("default_constructed_poly_vec_is_empty_with_max_align_equals_one",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v;
  REQUIRE(v.empty());
  REQUIRE(0 == v.size());
  REQUIRE(0 == v.sizes().first);
  REQUIRE(0 == v.sizes().second);
  REQUIRE(0 == v.capacity());
  REQUIRE(0 == v.capacities().first);
  REQUIRE(0 == v.capacities().second);
  REQUIRE(1 == v.max_align());
}

TEST_CASE("reserve_increases_capacity_with_default_align_as_max_align_t",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v;
  constexpr size_t n = 16;
  constexpr size_t avg_s = 64;
  v.reserve(n, avg_s);
  REQUIRE(v.empty());
  REQUIRE(0 == v.size());
  REQUIRE(0 == v.sizes().first);
  REQUIRE(0 == v.sizes().second);
  REQUIRE(n <= v.capacity());
  REQUIRE(n <= v.capacities().first);
  REQUIRE(n * avg_s <= v.capacities().second);
  REQUIRE(alignof(std::max_align_t) == v.max_align());
}

TEST_CASE("reserve_reallocates_capacity_when_max_align_changes",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v;
  constexpr size_t n = 16;
  constexpr size_t avg_s = 64;
  v.reserve(n, avg_s);
  auto old_data = v.data();
  v.reserve(n, avg_s, 32);
  REQUIRE(old_data != v.data());
}

TEST_CASE(
  "reserve_does_not_increase_capacity_when_size_is_less_than_current_capacity",
  "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v;
  constexpr size_t n = 16;
  constexpr size_t avg_s = 8;
  v.reserve(n, avg_s);
  const auto capacities = v.capacities();
  v.reserve(n / 2, avg_s);
  REQUIRE(capacities == v.capacities());
}

TEST_CASE("descendants_of_interface_can_be_pushed_back_into_the_vector",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v;
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  REQUIRE(2 == v.size());
}

TEST_CASE("is_not_copyable_with_no_cloning_policy", "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface,
                    std::allocator<Interface>,
                    estd::no_cloning_policy<Interface>>
    v{};
  v.reserve(2,
            std::max(sizeof(Impl1), sizeof(Impl2)),
            std::max(alignof(Impl1), alignof(Impl2)));
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  REQUIRE_THROWS_AS(v.push_back(Impl2()), estd::no_cloning_exception);
}

template<typename T>
bool
is_aligned_properly(T& obj)
{
  return reinterpret_cast<std::uintptr_t>(&obj) % alignof(T) == 0;
}

TEST_CASE("objects_are_allocated_with_proper_alignment",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v{};
  v.reserve(2,
            std::max(sizeof(Impl1), sizeof(Impl2)),
            std::max(alignof(Impl1), alignof(Impl2)));
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  REQUIRE(is_aligned_properly(static_cast<Impl1&>(v[0])));
  REQUIRE(is_aligned_properly(static_cast<Impl2&>(v[1])));
}

TEST_CASE("no_cloning_policy_gives_e_what", "[poly_vector_basic_tests]")
{
  bool throwed = false;
  estd::poly_vector<Interface,
                    std::allocator<Interface>,
                    estd::no_cloning_policy<Interface>>
    v{};
  try {
    v.reserve(1, sizeof(Impl1));
    v.push_back(Impl1(3.14));
    v.push_back(Impl1(3.14));
  } catch (const estd::no_cloning_exception& e) {
    std::string emsg = e.what();
    throwed = emsg.find("no_cloning_policy") != std::string::npos;
  }
  REQUIRE(throwed);
}

TEST_CASE("erase_from_begin_to_end_clears_the_vector",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v{};
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  auto it = v.erase(v.begin(), v.end());
  REQUIRE(it == v.end());
  REQUIRE(0 == v.size());
}

TEST_CASE("erase_can_be_called_with_any_valid_iterator_to_a_vector_elem",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v{};
  v.push_back(Impl1(3.14));
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  auto it = v.erase(v.begin(), v.begin() + 2);
  REQUIRE(it == v.begin());
  REQUIRE(3 == v.size());
  REQUIRE_NOTHROW(dynamic_cast<Impl2&>(v[0]));
  REQUIRE_NOTHROW(dynamic_cast<Impl1&>(v[1]));
  REQUIRE_NOTHROW(dynamic_cast<Impl2&>(v[2]));
}

TEST_CASE("erase_includes_end_remove_elems_from_end",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v{};
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  auto id = v[0].getId();
  auto it = v.erase(++v.begin(), v.end());

  REQUIRE(it == v.end());
  REQUIRE(1 == v.size());
  REQUIRE(id == v[0].getId());
}

TEST_CASE("erase_from_end_position_is_same_as_pop_back",
          "[poly_vector_basic_tests]")
{
  estd::poly_vector<Interface> v{};
  v.push_back(Impl1(3.14));
  v.push_back(Impl2());
  auto id = v[0].getId();
  v.erase(++v.begin());
  REQUIRE(1 == v.size());
  REQUIRE(id == v[0].getId());
}

TYPE_P_TEST_CASE("poly vector modifiers test",
                 "[poly_vector]",
                 CloningPolicy,
                 estd::virtual_cloning_policy<Interface>,
                 estd::delegate_cloning_policy<Interface>)
{
  using vector =
    estd::poly_vector<Interface, std::allocator<Interface>, CloningPolicy>;

  Impl1 obj1;
  Impl2T<CloningPolicy> obj2;
  vector v;
  v.push_back(obj1);
  v.push_back(obj2);

  SECTION("back accesses the last element")
  {
    Interface& obj = obj2;
    REQUIRE(obj == v.back());
  }
  SECTION("front accesses the first element")
  {
    Interface& obj = obj1;
    REQUIRE(obj == v.front());
  }
  SECTION("pop_back_removes_the_last_element")
  {
    REQUIRE(obj2 == v.back());
    v.pop_back();
    REQUIRE(obj1 == v.back());
  }
}

/*



TYPED_TEST_P(poly_vector_modifiers_test,
pop_back_keeps_capacity_but_adjustes_size)
{
    auto cap = this->v.capacities();
    
    this->v.pop_back();
    this->v.pop_back();
    
    EXPECT_EQ(cap, this->v.capacities());
    REQUIRE(this->v.empty());
    EXPECT_EQ(std::make_pair(size_t(0),size_t(0)), this->v.sizes());
}

TYPED_TEST_P(poly_vector_modifiers_test,
clear_destroys_all_elements_but_keeps_capacity)
{
    using Impl2 = Impl2T<TypeParam>;
    this->v.push_back(Impl2{});
    this->v.push_back(Impl1{6.28});
    auto cap = this->v.capacities();
    
    this->v.clear();
    
    EXPECT_EQ(cap, this->v.capacities());
    REQUIRE(this->v.empty());
    EXPECT_EQ(std::make_pair(size_t(0), size_t(0)), this->v.sizes());
}

TYPED_TEST_P(poly_vector_modifiers_test, at_operator_retrieves_nth_elem)
{
    EXPECT_EQ(static_cast<Interface&>(this->obj1), this->v[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), this->v[1]);
}

TYPED_TEST_P(poly_vector_modifiers_test,
at_operator_throws_out_of_range_error_on_overindexing)
{
    EXPECT_NO_THROW(this->v.at(1));
    EXPECT_THROW(this->v.at(2), std::out_of_range);
}

TYPED_TEST_P(poly_vector_modifiers_test, swap_swaps_the_contents_of_two_vectors)
{
    using Impl2 = Impl2T<TypeParam>;
    using vector = typename poly_vector_modifiers_test<TypeParam>::vector;
    vector v2;
    Impl1 obj1{ 6.28 };
    Impl2 obj2{};
    v2.push_back(obj1);
    v2.push_back(obj2);
    
    this->v.swap(v2);
    
    EXPECT_EQ(static_cast<Interface&>(obj1),this->v[0]);
    EXPECT_EQ(static_cast<Interface&>(obj2), this->v[1]);
    EXPECT_EQ(static_cast<Interface&>(this->obj1), v2[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), v2[1]);

}

TYPED_TEST_P(poly_vector_modifiers_test, copy_assignment_copies_contained_elems)
{
    using vector = typename poly_vector_modifiers_test<TypeParam>::vector;
    vector v2;

    v2 = this->v;

    EXPECT_EQ(static_cast<Interface&>(this->obj1), v2[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), v2[1]);
}

TYPED_TEST_P(poly_vector_modifiers_test, move_assignment_moves_contained_elems)
{
    using vector = typename poly_vector_modifiers_test<TypeParam>::vector;
    vector v2;
    auto size = v2.sizes();
    auto capacities = v2.capacities();

    v2 = std::move(this->v);

    EXPECT_EQ(static_cast<Interface&>(this->obj1), v2[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), v2[1]);
    EXPECT_EQ(size, this->v.sizes());
    EXPECT_EQ(capacities, this->v.capacities());

}

TYPED_TEST_P(poly_vector_modifiers_test,
strog_guarantee_when_there_is_an_exception_during_push_back_w_reallocation)
{

    this->v[1].set_throw_on_copy_construction(true);
    while (this->v.capacity() - this->v.size() > 0) {
        this->v.push_back(Impl1{});
    }
    if (!std::is_same<TypeParam, estd::delegate_cloning_policy<Interface>>{}) {
        std::vector<uint8_t>
ref_storage(static_cast<uint8_t*>(this->v.data().first),
static_cast<uint8_t*>(this->v.data().second)); auto data = this->v.data(); auto
size = this->v.sizes(); auto caps = this->v.capacities();

        EXPECT_THROW(this->v.push_back(Impl2{}), std::exception);

        EXPECT_EQ(data, this->v.data());
        EXPECT_EQ(size, this->v.sizes());
        EXPECT_EQ(caps, this->v.capacities());
        REQUIRE(0 == memcmp(ref_storage.data(), this->v.data().first,
ref_storage.size()));
    }
    else {
        EXPECT_NO_THROW(this->v.push_back(Impl2{}));
    }
}

TYPED_TEST_P(poly_vector_modifiers_test,
basic_guarantee_when_there_is_an_exception_during_erase_and_last_element_is_not_included_in_erased_range)
{
    this->v.push_back(Impl1{});
    this->v.push_back(Impl1{});
    this->v.push_back(Impl1{});
    this->v.push_back(Impl1{});
    this->v.push_back(Impl2{});
    this->v.push_back(Impl1{});
    this->v.push_back(Impl2{});

    auto start = this->v.begin() + 3;
    auto end = start + 1;

    if (!std::is_same<TypeParam, estd::delegate_cloning_policy<Interface>>{}) {
        this->v.at(5).set_throw_on_copy_construction(true);
        auto old_sizes = this->v.sizes();
        EXPECT_THROW(this->v.erase(start, end), std::exception);
        EXPECT_LT(this->v.size(), old_sizes.first);
    }
    else {
        EXPECT_NO_THROW(this->v.erase(start, end));
    }
}

TYPED_TEST_P( poly_vector_modifiers_test, insert_at_end )
{
    auto o = Impl1();
    auto id = o.getId();
    auto old_size = this->v.size();
    this->v.insert( this->v.end(), std::move( o ) );
    EXPECT_EQ( id, this->v.back().getId() );
    EXPECT_EQ( old_size + 1, this->v.size() );
}

REGISTER_TYPED_TEST_CASE_P(poly_vector_modifiers_test,
    back_accesses_the_last_element,
    front_accesses_the_first_element,
    pop_back_removes_the_last_element,
    pop_back_keeps_capacity_but_adjustes_size,
    clear_destroys_all_elements_but_keeps_capacity,
    at_operator_retrieves_nth_elem,
    at_operator_throws_out_of_range_error_on_overindexing,
    swap_swaps_the_contents_of_two_vectors,
    copy_assignment_copies_contained_elems,
    move_assignment_moves_contained_elems,
    strog_guarantee_when_there_is_an_exception_during_push_back_w_reallocation,
    basic_guarantee_when_there_is_an_exception_during_erase_and_last_element_is_not_included_in_erased_range,
    insert_at_end
);

INSTANTIATE_TYPED_TEST_CASE_P(poly_vector, poly_vector_modifiers_test,
CloneTypes);

*/
