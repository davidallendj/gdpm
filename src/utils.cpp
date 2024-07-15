
#include "utils.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "error.hpp"
#include "log.hpp"

#include "csv2/reader.hpp"


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
#include <curl/curl.h>

namespace gdpm::utils{

	

	bool to_bool(const string& s){
		return to_lower(s) == "true";
	}

	std::vector<string> split_lines(const string& contents){
		using namespace csv2;
		csv2::Reader<
			delimiter<'\n'>,
			quote_character<'"'>,
			first_row_is_header<false>,
			trim_policy::trim_whitespace
		> csv;
		std::vector<string> lines;
		if(csv.parse(contents)){
			for(const auto& row : csv){
				for(const auto& cell : row){
					lines.emplace_back(cell.read_view());
				}
			}
		}
		return lines;
	}

	
	#if (GDPM_READFILE_IMPL == 0)
		string readfile(const string& path){
			constexpr auto read_size = std::size_t{4096};
			auto stream = std::ifstream{path.data()};
			stream.exceptions(std::ios_base::badbit);

			auto out = string{};
			auto buf = string(read_size, '\0');
			while (stream.read(& buf[0], read_size)) {
				out.append(buf, 0, stream.gcount());
			}
			out.append(buf, 0, stream.gcount());
			return out;
		}
#elif(GDPM_READFILE_IMPL == 1)
		
	string readfile(const string& path){
		std::ifstream ifs(path);
		return string(
			(std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>())
		);
	}
#elif(GDPM_READFILE_IMPL == 2)
	string readfile(const string& path){
		std::ifstream ifs(path);
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		return buffer.str();
	}
#endif

	string to_lower(const string& s){
		string copy = s;
		std::transform(copy.begin(), copy.end(), copy.begin(), tolower);
		return copy;
	}

	string trim(const string& s){
		return trim_right(trim_left(s));
	}

	string trim_left(const string& s){		
		return trim_left(s, constants::WHITESPACE);
	}

	string trim_left(
		const string& s, 
		const string& ref
	){
		size_t start = s.find_first_not_of(ref);
		return (start == string::npos) ? "" : s.substr(start);	
	}

	string trim_right(const string& s){
		return trim_right(s, constants::WHITESPACE);
	}

	string trim_right(
		const string& s, 
		const string& ref
	){
		size_t end =  s.find_last_not_of(ref);
		return (end == string::npos) ? "" : s.substr(0, end + 1);
	}

	std::vector<string> parse_lines(const string &s){
		string line;
		std::vector<string> result;
		std::stringstream ss(s);
		while(std::getline(ss, line)){
			result.emplace_back(line);
		}
		return result;
	}

	string replace_first(
		const string &s, 
		const string &from, 
		const string &to
	){
		string copy = s; // make string copy
		size_t pos = copy.find(from);
		if(pos == string::npos)
			return copy;
		return copy.replace(pos, from.length(), to);
	}

	string replace_all(
		const string& s, 
		const string& from, 
		const string& to
	){
		string copy = s; // make string copy
		size_t pos = 0;
		while((pos = copy.find(from, pos)) != string::npos){
			copy.replace(pos, s.length(), to);
			pos += to.length();
		}
		return copy;
	}

	/* Ref: https://gist.github.com/mobius/1759816 */
	error extract_zip(
		const char *archive, 
		const char *dest, 
		int verbose
	){
		constexpr int SIZE = 1024;
		struct zip *za;
		struct zip_file *zf;
		struct zip_stat sb;
		char buf[SIZE];
		int err;
		int i, len, fd;
		zip_uint64_t sum;

		std::filesystem::path path(archive);
		log::info_n("Extracting \"{}\" archive...", path.filename().string());
		if((za = zip_open(path.c_str(), ZIP_RDONLY, &err)) == NULL){
			zip_error_to_str(buf, sizeof(buf), err, errno);
			log::println("");
			return log::error_rc(error(
				ec::LIBZIP_ERR, 
				std::format("utils::extract_zip(): can't open zip archive \"{}\": {}", path.filename().string(), buf))
			);
		}

		for(i = 0; i < zip_get_num_entries(za, 0); i++){
			if(zip_stat_index(za, i, 0, &sb) == 0){
				len = strlen(sb.name);
				if(verbose > 1){
					log::println("utils::extract_zip(): {}, size: {}", sb.name, sb.size);
				}
				string path{dest};
				path += sb.name;
				if(sb.name[len-1] == '/'){
					std::filesystem::create_directory(path);
				} else {
					zf = zip_fopen_index(za, i, 0);
					if(!zf){
						return log::error_rc(error(ec::LIBZIP_ERR, 
							"utils::extract_zip(): zip_fopen_index() failed.")
						);
					}
#ifdef _WIN32
					fd = open(sb.name, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 0644);
#else
					fd = open(path.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0644);
#endif
					if(fd < 0){
						log::error_rc(error(ec::LIBZIP_ERR,
							std::format("utils::extract_zip(): open() failed. (path: {}, fd={})", path, fd))
						);
					}

					sum = 0;
					while(sum != sb.size){
						len = zip_fread(zf, buf, 100);
						if(len < 0){
							return log::error_rc(error(
								ec::LIBZIP_ERR,
								std::format("utils::extract_zip(): zip_fread() returned len < 0 (len={})", len))
							);
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
			return log::error_rc(error(ec::LIBZIP_ERR,
				std::format("utils::extract_zip: can't close zip archive '{}'", archive))
			);
		}
		return error();
	}

	string prompt_user(const char *message){
		log::print("{} ", message);
		string input;
		getline(std::cin, input);
		return input;
	}
	
	bool prompt_user_yn(const char *message){
		string input{""};
		while( input != "y" && input != "n" ){
			input = to_lower(utils::prompt_user(message));
			bool is_default = (input == "\0" || input == "\n" || input == "\r\n" || input.empty());
			input = is_default ? "y" : input;
		}
		return input == "y";
	}

	void delay(std::chrono::milliseconds millis){
		using namespace std::this_thread;
		using namespace std::chrono_literals;
		using std::chrono::system_clock;

		sleep_for(millis);
		// sleep_until(system_clock::now() + millis);
	}

	string join(
		const std::vector<string>& target,
		const string& delimiter
	){
		string o;
		std::for_each(
			target.begin(), 
			target.end(), 
			[&o, &delimiter](const string& s){
				o += s + delimiter;
			}
		);
		o = trim_right(o, delimiter);
		return o;
	}

	string join(
		const std::unordered_map<string, string>& target,
		const string& prefix,
		const string& delimiter
	){
		string o;
		std::for_each(
			target.begin(),
			target.end(),
			[&o, &prefix, &delimiter](const std::pair<string, string>& p){
				o += prefix + p.first + ": " + p.second + delimiter;
			}
		);
		o = trim_right(o, delimiter);
		return o;
	}

	string convert_size(long size){
		int digit = 0;
		while(size > 1000){
			size /= 1000;
			digit += 1;
		}
		string s = std::to_string(size);
		switch(digit){
			case 0: return s + " B";
			case 1: return s + " KB";
			case 2: return s + " MB";
			case 3: return s + " GB";
			case 4: return s + " TB";
			case 5: return s + " PB";
		}
		return std::to_string(size);
	}	

	namespace json {
		string from_array(
			const std::set<string>& a, 
			const string& prefix
		){
			string o{"["};
			for(const string& src : a)
				o += prefix + "\t\"" + src + "\",";
			if(o.back() == ',')
				o.pop_back();
			o += prefix + "]";
			return o;
		};


		string from_object(
			const std::unordered_map<string, string>& m, 
			const string& prefix, 
			const string& spaces
		){
			string o{"{"};
			std::for_each(m.begin(), m.end(), 
				[&o, &prefix, &spaces](const std::pair<string, string>& p){
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
