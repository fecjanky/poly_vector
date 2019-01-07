#include <type_traits>

int main(){
	static_assert(std::disjunction<std::false_type,std::true_type>::value, "should compile");
	return 0;
}