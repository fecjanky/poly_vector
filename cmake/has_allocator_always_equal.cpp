#include <type_traits>
#include <memory>

int main(){
	static_assert(std::is_convertible<
        decltype(std::allocator_traits<std::allocator<int>>::is_always_equal::value), 
		bool>::value,"should compile");

	return 0;
}