#include <type_traits>

int main()
{
    std::decay_t<int*>                   decay{};
    std::enable_if_t<1, int>             enable_if{};
    std::add_lvalue_reference_t<int>     ref = enable_if;
    std::conditional_t<false, void, int> conditional{};
    std::remove_const_t<const int>       remove_const{};
    std::add_const_t<int>                add_const{ 2 };
    return 0;
}