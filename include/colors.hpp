#pragma once

#include "colors.hpp"
#include <string>

// #define GDPM_ENABLE_COLORS 1
#if GDPM_ENABLE_COLORS == 1
	constexpr const char *GDPM_COLOR_BLACK 			= "\033[0;30m";
	constexpr const char *GDPM_COLOR_BLUE 			= "\033[0;34m";
	constexpr const char *GDPM_COLOR_GREEN 			= "\033[0;32m";
	constexpr const char *GDPM_COLOR_CYAN 			= "\033[0;36m";
	constexpr const char *GDPM_COLOR_RED 			= "\033[0;31m";
	constexpr const char *GDPM_COLOR_PURPLE 		= "\033[0;35m";
	constexpr const char *GDPM_COLOR_BROWN 			= "\033[0;33m";
	constexpr const char *GDPM_COLOR_GRAY 			= "\033[0;37m";
	constexpr const char *GDPM_COLOR_DARK_GRAY 		= "\033[0;30m";
	constexpr const char *GDPM_COLOR_LIGHT_BLUE 	= "\033[0;34m";
	constexpr const char *GDPM_COLOR_LIGHT_GREEN 	= "\033[0;32m";
	constexpr const char *GDPM_COLOR_LIGHT_CYAN 	= "\033[0;36m";
	constexpr const char *GDPM_COLOR_LIGHT_RED 		= "\033[0;31m";
	constexpr const char *GDPM_COLOR_LIGHT_PURPLE 	= "\033[0;35m";
	constexpr const char *GDPM_COLOR_YELLOW 		= "\033[0;33m";
	constexpr const char *GDPM_COLOR_WHITE 			= "\033[0;37m";
	constexpr const char *GDPM_COLOR_RESET 			= GDPM_COLOR_WHITE;

#else
	constexpr const char *GDPM_COLOR_BLACK 			= "";
	constexpr const char *GDPM_COLOR_BLUE 			= "";
	constexpr const char *GDPM_COLOR_GREEN 			= "";
	constexpr const char *GDPM_COLOR_CYAN 			= "";
	constexpr const char *GDPM_COLOR_RED 			= "";
	constexpr const char *GDPM_COLOR_PURPLE 		= "";
	constexpr const char *GDPM_COLOR_BROWN 			= "";
	constexpr const char *GDPM_COLOR_GRAY 			= "";
	constexpr const char *GDPM_COLOR_DARK_GRAY 		= "";
	constexpr const char *GDPM_COLOR_LIGHT_BLUE 	= "";
	constexpr const char *GDPM_COLOR_LIGHT_GREEN 	= "";
	constexpr const char *GDPM_COLOR_LIGHT_CYAN 	= "";
	constexpr const char *GDPM_COLOR_LIGHT_RED 		= "";
	constexpr const char *GDPM_COLOR_LIGHT_PURPLE 	= "";
	constexpr const char *GDPM_COLOR_YELLOW 		= "";
	constexpr const char *GDPM_COLOR_WHITE 			= "";
	constexpr const char *GDPM_COLOR_RESET 			= GDPM_COLOR_WHITE;
#endif

constexpr const char *GDPM_COLOR_LOG_RESET 			= GDPM_COLOR_WHITE;
constexpr const char *GDPM_COLOR_LOG_INFO 			= GDPM_COLOR_LOG_RESET;
constexpr const char *GDPM_COLOR_LOG_ERROR 			= GDPM_COLOR_RED;
constexpr const char *GDPM_COLOR_LOG_DEBUG 			= GDPM_COLOR_YELLOW;
constexpr const char *GDPM_COLOR_LOG_WARNING 		= GDPM_COLOR_YELLOW;

namespace gdpm::color{
	inline std::string from_string(const std::string& color_name){
		if		(color_name == "red"){ return GDPM_COLOR_RED; 			}
		else if	(color_name == "yellow"){ return GDPM_COLOR_YELLOW; 		}
		else if	(color_name == "green"){ return GDPM_COLOR_GREEN; 		}
		else if	(color_name == "blue"){ return GDPM_COLOR_BLUE; 			}
		else if	(color_name == "brown"){ return GDPM_COLOR_BROWN; 		}
		else if	(color_name == "gray"){ return GDPM_COLOR_GRAY; 			}
		else if	(color_name == "black"){ return GDPM_COLOR_BLACK; 		}
		else if	(color_name == "purple"){ return GDPM_COLOR_PURPLE; 		} 
		else if	(color_name == "gray"){ return GDPM_COLOR_DARK_GRAY; 	}
		else if	(color_name == "light-blue"){ return GDPM_COLOR_LIGHT_BLUE; 	}
		else if	(color_name == "light-green"){ return GDPM_COLOR_LIGHT_GREEN; 	}
		else if	(color_name == "light-cyan"){ return GDPM_COLOR_LIGHT_CYAN; 	}
		else if	(color_name == "light-red"){ return GDPM_COLOR_LIGHT_RED; 	}
		else if	(color_name == "light-purple"){ return GDPM_COLOR_LIGHT_PURPLE; 	}
		return "";
	}
}