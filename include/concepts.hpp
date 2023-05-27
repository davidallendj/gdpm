#pragma once
#include <string>

namespace gdpm::concepts{
	template <typename...Args> concept RequireMinArgs = requires (std::size_t min){ sizeof...(Args) > min; };
}