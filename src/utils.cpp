
#include "utils.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "indicators/indeterminate_progress_bar.hpp"
#include "indicators/dynamic_progress.hpp"
#include "indicators/progress_bar.hpp"
#include "indicators/block_progress_bar.hpp"
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

	using namespace indicators;
	BlockProgressBar bar {
		option::BarWidth{50},
		// option::Start{"["},
		// option::Fill{"="},
		// option::Lead{">"},
		// option::Remainder{" "},
		// option::End{"]"},
		option::PrefixText{"Downloading file "},
		option::PostfixText{""},
		option::ForegroundColor{Color::green},
		option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
	};
		// option::ShowElapsedTime{true},
		// option::ShowRemainingTime{true},
	IndeterminateProgressBar bar_unknown {
		option::BarWidth{50},
		option::Start{"["},
		option::Fill{"."},
		option::Lead{"<==>"},
		option::PrefixText{"Downloading file "},
		option::End{"]"},
		option::PostfixText{""},
		option::ForegroundColor{Color::green},
		option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
	};

	std::vector<std::string> split_lines(const std::string& contents){
		using namespace csv2;
		csv2::Reader<
			delimiter<'\n'>,
			quote_character<'"'>,
			first_row_is_header<false>,
			trim_policy::trim_whitespace
		> csv;
		std::vector<std::string> lines;
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
		const std::string &s, 
		const std::string &from, 
		const std::string &to
	){
		std::string copy = s; // make string copy
		size_t pos = copy.find(from);
		if(pos == std::string::npos)
			return copy;
		return copy.replace(pos, from.length(), to);
	}

	std::string replace_all(
		const std::string& s, 
		const std::string& from, 
		const std::string& to
	){
		std::string copy = s; // make string copy
		size_t pos = 0;
		while((pos = copy.find(from, pos)) != std::string::npos){
			copy.replace(pos, s.length(), to);
			pos += to.length();
		}
		return copy;
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

		// log::info_n("Extracting package contents to '{}'...", dest);
		log::info_n("Extracting package contents...");
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
		// std::cin >> input;
		getline(std::cin, input);
		return input;
	}
	
	bool prompt_user_yn(const char *message){
		std::string input{""};
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

	std::string convert_size(long size){
		int digit = 0;
		while(size > 1000){
			size /= 1000;
			digit += 1;
		}
		std::string s = std::to_string(size);
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


	namespace curl {
		size_t write_to_buffer(
			char *contents, 
			size_t size, 
			size_t nmemb, 
			void *userdata
		){
			size_t realsize = size * nmemb;
			struct memory_buffer *m = (struct memory_buffer*)userdata;

			m->addr = (char*)realloc(m->addr, m->size + realsize + 1);
			if(m->addr == nullptr){
				/* Out of memory */
				fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
				return 0;
			}

			memcpy(&(m->addr[m->size]), contents, realsize);
			m->size += realsize;
			m->addr[m->size] = 0;

			return realsize;
		}

		size_t write_to_stream(
			char *ptr, 
			size_t size, 
			size_t nmemb, 
			void *userdata
		){
			if(nmemb == 0)
				return 0;
						
			return fwrite(ptr, size, nmemb, (FILE*)userdata);
		}

		int show_progress(
			void *ptr,
			curl_off_t total_download,
			curl_off_t current_downloaded,
			curl_off_t total_upload,
			curl_off_t current_upload
		){

			if(current_downloaded >= total_download)
				return 0;
			using namespace indicators;
			show_console_cursor(false);
			if(total_download != 0){
				// double percent = std::floor((current_downloaded / (total_download)) * 100);
				bar.set_option(option::MaxProgress{total_download});
				bar.set_progress(current_downloaded);
				bar.set_option(option::PostfixText{
						convert_size(current_downloaded) + " / " + 
						convert_size(total_download)
				});
				if(bar.is_completed()){
					bar.set_option(option::PrefixText{"Download completed."});
					bar.mark_as_completed();
				}
			} else {
				if(bar_unknown.is_completed()){
					bar.set_option(option::PrefixText{"Download completed."});
					bar.mark_as_completed();
				} else {
					bar.tick();
					bar_unknown.set_option(
						option::PostfixText(std::format("{}", convert_size(current_downloaded)))
					);

				}
			}
			show_console_cursor(true);
			memory_buffer *m = (memory_buffer*)ptr;
			return 0;
		}
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
