
#include "utils.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "log.hpp"


#include <asm-generic/errno-base.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <readline/chardefs.h>
#include <readline/readline.h>

#include <string>
#include <thread>
#include <unordered_map>
#include <zip.h>

namespace gdpm::utils{

	
	#if (GDPM_READFILE_IMPL == 0)
		std::string readfile(const std::string& path){
			constexpr auto read_size = std::size_t{4096};
			auto stream = std::ifstream{path.data()};
			stream.exceptions(std::ios_base::badbit);

			auto out = std::string{};
			auto buf = std::string(read_size, '\0');
			while (stream.read(& buf[0], read_size)) {
				out.append(buf, 0, stream.gcount());
			}
			out.append(buf, 0, stream.gcount());
			return out;
		}
#elif(GDPM_READFILE_IMPL == 1)
		
	std::string readfile(const std::string& path){
		std::ifstream ifs(path);
		return std::string(
			(std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>())
		);
	}
#elif(GDPM_READFILE_IMPL == 2)
	std::string readfile(const std::string& path){
		std::ifstream ifs(path);
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		return buffer.str();
	}
#endif

	std::string to_lower(const std::string& s){
		std::string copy = s;
		std::transform(copy.begin(), copy.end(), copy.begin(), tolower);
		return copy;
	}

	std::string trim(const std::string& s){
		return trim_right(trim_left(s));
	}

	std::string trim_left(const std::string& s){		
		return trim_left(s, constants::WHITESPACE);
	}

	std::string trim_left(
		const std::string& s, 
		const std::string& ref
	){
		size_t start = s.find_first_not_of(ref);
		return (start == std::string::npos) ? "" : s.substr(start);	
	}

	std::string trim_right(const std::string& s){
		return trim_right(s, constants::WHITESPACE);
	}

	std::string trim_right(
		const std::string& s, 
		const std::string& ref
	){
		size_t end =  s.find_last_not_of(ref);
		return (end == std::string::npos) ? "" : s.substr(0, end + 1);
	}

	std::vector<std::string> parse_lines(const std::string &s){
		std::string line;
		std::vector<std::string> result;
		std::stringstream ss(s);
		while(std::getline(ss, line)){
			result.emplace_back(line);
		}
		return result;
	}

	std::string replace_first(
		std::string &s, 
		const std::string &from, 
		const std::string &to
	){
		size_t pos = s.find(from);
		if(pos == std::string::npos)
			return s;
		return s.replace(pos, from.length(), to);
	}

	std::string replace_all(
		std::string& s, 
		const std::string& from, 
		const std::string& to
	){
		size_t pos = 0;
		while((pos = s.find(from, pos)) != std::string::npos){
			s.replace(pos, s.length(), to);
			pos += to.length();
		}
		return s;
	}

	/* Ref: https://gist.github.com/mobius/1759816 */
	int extract_zip(
		const char *archive, 
		const char *dest, 
		int verbose
	){
		constexpr const char *prog = "gpdm";
		struct zip *za;
		struct zip_file *zf;
		struct zip_stat sb;
		char buf[100];
		int err;
		int i, len, fd;
		zip_uint64_t sum;	

		log::info_n("Extracting package contents to '{}'...", dest);
		if((za = zip_open(archive, 0, &err)) == nullptr){
			zip_error_to_str(buf, sizeof(buf), err, errno);
			log::error("{}: can't open zip archive {}: {}", prog, archive, buf);
			return 1;
		}

		for(i = 0; i < zip_get_num_entries(za, 0); i++){
			if(zip_stat_index(za, i, 0, &sb) == 0){
				len = strlen(sb.name);
				if(verbose > 1){
					log::print("{}, ", sb.name);
					log::println("size: {}, ", sb.size);
				}
				std::string path{dest};
				path += sb.name;
				if(sb.name[len-1] == '/'){
					// safe_create_dir(sb.name);
					std::filesystem::create_directory(path);
				} else {
					zf = zip_fopen_index(za, i, 0);
					if(!zf){
						log::error("extract_zip: zip_fopen_index() failed.");
						return 100;
					}
#ifdef _WIN32
					fd = open(sb.name, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 0644);
#else
					fd = open(path.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0644);
#endif
					if(fd < 0){
						log::error("extract_zip: open() failed. (path: {}, fd={})", path, fd);
						return 101;
					}

					sum = 0;
					while(sum != sb.size){
						len = zip_fread(zf, buf, 100);
						if(len < 0){
							log::error("extract_zip: zip_fread() returned len < 0 (len={})", len);
							return 102;
						}
						write(fd, buf, len);
						sum += len;
					}
					close(fd);
					zip_fclose(zf);
				}
			} else {
				log::println("File[{}] Line[{}]\n", __FILE__, __LINE__);
			}
		}

		if(zip_close(za) == -1){
			log::error("{}: can't close zip archive '{}'", prog, archive);
			return 1;
		}
		log::println("Done.");
		return 0;
	}

	std::string prompt_user(const char *message){
		log::print("{} ", message);
		std::string input;
		std::cin >> input;
		return input;
	}
	
	bool prompt_user_yn(const char *message){
		std::string input{"y"};
		do{
			input = utils::prompt_user(message);
			to_lower(input);
			input = (input == "\0" || input == "\n" || input.empty()) ? "y" : input;
		}while( !std::cin.fail() && input != "y" && input != "n");
		return input == "y";
	}

	void delay(std::chrono::milliseconds millis){
		using namespace std::this_thread;
		using namespace std::chrono_literals;
		using std::chrono::system_clock;

		sleep_for(millis);
		// sleep_until(system_clock::now() + millis);
	}

	std::string join(
		const std::vector<std::string>& target,
		const std::string& delimiter
	){
		std::string o;
		std::for_each(
			target.begin(), 
			target.end(), 
			[&o, &delimiter](const std::string& s){
				o += s + delimiter;
			}
		);
		o = trim_right(o, delimiter);
		return o;
	}

	std::string join(
		const std::unordered_map<std::string, std::string>& target,
		const std::string& prefix,
		const std::string& delimiter
	){
		std::string o;
		std::for_each(
			target.begin(),
			target.end(),
			[&o, &prefix, &delimiter](const std::pair<std::string, std::string>& p){
				o += prefix + p.first + ": " + p.second + delimiter;
			}
		);
		o = trim_right(o, delimiter);
		return o;
	}

	namespace json {

		std::string from_array(
			const std::set<std::string>& a, 
			const std::string& prefix
		){
			std::string o{"["};
			for(const std::string& src : a)
				o += prefix + "\t\"" + src + "\",";
			if(o.back() == ',')
				o.pop_back();
			o += prefix + "]";
			return o;
		};


		std::string from_object(
			const std::unordered_map<std::string, std::string>& m, 
			const std::string& prefix, 
			const std::string& spaces
		){
			std::string o{"{"};
			std::for_each(m.begin(), m.end(), 
				[&o, &prefix, &spaces](const std::pair<std::string, std::string>& p){
					o += std::format("{}\t\"{}\":{}\"{}\",", prefix, p.first, spaces, p.second);
				}
			);
			if(o.back() == ',')
				o.pop_back();
			o += prefix + "}";
			return o;
		};
	}

} // namespace gdpm::utils
