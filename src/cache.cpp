
#include "cache.hpp"
#include "log.hpp"
#include "constants.hpp"
#include "package_manager.hpp"
#include "utils.hpp"
#include <filesystem>
#include <string>


namespace gdpm::cache{
	int create_package_database(const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg;

		/* Check and make sure directory is created before attempting to open */
		if(!std::filesystem::exists(cache_path)){
			log::info("Creating cache directories...{}", cache_path);
			std::filesystem::create_directories(cache_path);
		}

		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("create_package_database.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return rc;
		}

		std::string sql = "CREATE TABLE IF NOT EXISTS " +
							table_name + "("
							"id INTEGER PRIMARY KEY AUTOINCREMENT,"
							"asset_id		INT		NOT NULL,"
							"type			INT		NOT NULL,"
							"title			TEXT	NOT NULL,"
							"author			TEXT	NOT NULL,"
							"author_id		INT		NOT NULL,"
							"version		TEXT	NOT NULL,"
							"godot_version	TEXT	NOT NULL,"
							"cost			TEXT	NOT NULL,"
							"description	TEXT	NOT NULL,"
							"modify_date	TEXT	NOT NULL,"
							"support_level	TEXT	NOT NULL,"
							"category		TEXT	NOT NULL,"
							"remote_source	TEXT	NOT NULL,"
							"download_url	TEXT	NOT NULL,"
							"download_hash	TEXT	NOT NULL,"
							"is_installed	TEXT	NOT NULL,"
							"install_path	TEXT	NOT NULL);";

		// rc = sqlite3_prepare_v2(db, "SELECT", -1, &res, 0);
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			// log::error("Failed to fetch data: {}\n", sqlite3_errmsg(db));
			log::error("create_package_database.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return rc;
		}
		sqlite3_close(db);
		return 0;
	}


	int insert_package_info(const std::vector<package_info>& packages, const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;

		/* Prepare values to use in sql statement */
		std::string sql{"BEGIN TRANSACTION; "};
		for(const auto& p : packages){
			sql += "INSERT INTO " + table_name + " (" GDPM_PACKAGE_CACHE_COLNAMES ") ";
			sql += "VALUES (" + to_values(p) + "); ";
		}
		sql += "COMMIT;";
		// log::println("{}", sql);
		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("insert_package_info.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return rc;
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			log::error("insert_package_info.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return rc;
		}
		sqlite3_close(db);
		return 0;
	}


	std::vector<package_info> get_package_info_by_id(const std::vector<size_t>& package_ids, const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		size_t p_size = 0;
		std::vector<package_info> p_vector;
		std::string sql{"BEGIN TRANSACTION;\n"};

		auto callback = [](void *data, int argc, char **argv, char **colnames){
			// log::error("{}", (const char*)data);
			// p_data *_data = (p_data*)data;
			std::vector<package_info> *_p_vector = (std::vector<package_info>*) data;
			package_info p{
				.asset_id 			= std::stoul(argv[1]),
				.type 				= argv[2],
				.title 				= argv[3],
				.author 			= argv[4],
				.author_id 			= std::stoul(argv[5]),
				.version 			= argv[6],
				.godot_version 		= argv[7],
				.cost 				= argv[8],
				.description 		= argv[9],
				.modify_date 		= argv[10],
				.support_level 		= argv[11],
				.category			= argv[12],
				.remote_source		= argv[13],
				.download_url		= argv[14],
				.download_hash 		= argv[15],
				.is_installed		= static_cast<bool>(std::stoi(argv[16])),
				.install_path		= argv[17]
			};
			_p_vector->emplace_back(p);
			return 0;
		};

		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("get_package_info_by_id.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return {};
		}
		
		for(const auto& p_id : package_ids){
			sql += "SELECT * FROM " + table_name + " WHERE asset_id=" + fmt::to_string(p_id)+ ";\n";
		}
		sql += "COMMIT;\n";
		rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&p_vector, &errmsg);
		if(rc != SQLITE_OK){
			log::error("get_package_info_by_id.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return {};
		}
		sqlite3_close(db);
		return p_vector;
	}


	std::vector<package_info> get_package_info_by_title(const std::vector<std::string>& package_titles, const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		std::vector<package_info> p_vector;

		auto callback = [](void *data, int argc, char **argv, char **colnames){
			if(argc <= 0)
				return 1;
			std::vector<package_info> *_p_vector = (std::vector<package_info>*)data;
			// log::println("get_package_info_by_title.callback.argv: \n\t{}\n\t{}\n\t{}\n\t{}\n\t{}", argv[0], argv[1], argv[2],argv[3], argv[4]);
			package_info p{
				.asset_id 			= std::stoul(argv[1]),
				.type 				= argv[2],
				.title 				= argv[3],
				.author 			= argv[4],
				.author_id 			= std::stoul(argv[5]),
				.version 			= argv[6],
				.godot_version 		= argv[7],
				.cost 				= argv[8],
				.description 		= argv[9],
				.modify_date 		= argv[10],
				.support_level 		= argv[11],
				.category			= argv[12],
				.remote_source		= argv[13],
				.download_url		= argv[14],
				.download_hash 		= argv[15],
				.is_installed		= static_cast<bool>(std::stoi(argv[16])),
				.install_path		= argv[17]
			};
			_p_vector->emplace_back(p);
			return 0;
		};

		/* Check to make sure the directory is there before attempting to open */
		if(!std::filesystem::exists(cache_path))
			std::filesystem::create_directories(cache_path);

		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("get_package_info_by_title.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return {};
		}

		std::string sql{"BEGIN TRANSACTION;"};
		for(const auto& p_title : package_titles){
			sql += "SELECT * FROM " + table_name + " WHERE title='" + p_title + "';";
		}
		sql += "COMMIT;";
		// log::println(sql);
		rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&p_vector, &errmsg);
		if(rc != SQLITE_OK){
			log::error("get_package_info_by_title.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return {};
		}
		sqlite3_close(db);
		return p_vector;
	}


	std::vector<package_info> get_installed_packages(const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		std::vector<package_info> p_vector;
		std::string sql{"BEGIN TRANSACTION;"};

		auto callback = [](void *data, int argc, char **argv, char **colnames){
			std::vector<package_info> *_p_vector = (std::vector<package_info>*) data;
			package_info p{
				.asset_id 			= std::stoul(argv[1]),
				.type 				= argv[2],
				.title 				= argv[3],
				.author 			= argv[4],
				.author_id 			= std::stoul(argv[5]),
				.version 			= argv[6],
				.godot_version 		= argv[7],
				.cost 				= argv[8],
				.description 		= argv[9],
				.modify_date 		= argv[10],
				.support_level 		= argv[11],
				.category			= argv[12],
				.remote_source		= argv[13],
				.download_url		= argv[14],
				.download_hash 		= argv[15],
				.is_installed		= static_cast<bool>(std::stoi(argv[16])),
				.install_path		= argv[17]
			};
			_p_vector->emplace_back(p);
			return 0;
		};

		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("get_installed_packages.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return {};
		}

		sql += "SELECT * FROM " + table_name + " WHERE is_installed=1; COMMIT;";
		rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&p_vector, &errmsg);
		if(rc != SQLITE_OK){
			log::error("get_installed_packages.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return {};
		}
		sqlite3_close(db);
		return p_vector;
	}


	int update_package_info(const std::vector<package_info>& packages, const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		
		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("update_package_info.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return rc;
		}
		
		std::string sql;
		for(const auto& p : packages){
			sql += "UPDATE " + table_name + " SET "
			" asset_id=" + fmt::to_string(p.asset_id) + ", "
			" type='" + p.type + "', "
			" title='" + p.title + "', "
			" author='" + p.author + "', " + 
			" author_id=" + fmt::to_string(p.author_id) + ", "
			" version='" + p.version + "', " + 
			" godot_version='" + p.godot_version + "', " +
			" cost='" + p.cost + "', " +
			" description='" + p.description + "', " +
			" modify_date='" + p.modify_date + "', " +
			" support_level='" + p.support_level + "', " +
			" category='" + p.category + "', " +
			" remote_source='" + p.remote_source + "', " +
			" download_url='" + p.download_url + "', " + 
			" download_hash='" + p.download_hash + "', " +
			" is_installed=" + fmt::to_string(p.is_installed) + ", " 
			" install_path='" + p.install_path + "'"
			// " dependencies='" + p.dependencies + "'"
			" WHERE title='" + p.title + "' AND asset_id=" + fmt::to_string(p.asset_id)
			+ ";\n";
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			log::error("update_package_info.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return rc;
		}
		sqlite3_close(db);
		return 0;
	}


	int delete_packages(const std::vector<std::string>& package_titles, const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		std::string sql;

		int rc = sqlite3_open(cache_path, &db);
		if(rc != SQLITE_OK){
			log::error("delete_packages.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return rc;
		}

		for(const auto& p_title : package_titles){
			sql += "DELETE FROM " + table_name + " WHERE title='"
			+ p_title + "';\n";
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			log::error("delete_packages.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return rc;
		}
		sqlite3_close(db);
		return 0;
	}


	int delete_packages(const std::vector<size_t>& package_ids, const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		std::string sql;

		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("delete_packages.sqlite3_open(): {}", errmsg);
			sqlite3_close(db);
			return rc;
		}

		for(const auto& p_id : package_ids){
			sql += "DELETE FROM " + table_name + " WHERE asset_id="
			+ fmt::to_string(p_id) + ";\n";
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			log::error("delete_packages.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return rc;
		}
		sqlite3_close(db);
		return 0;
	}


	int drop_package_database(const std::string& cache_path, const std::string& table_name){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		std::string sql{"DROP TABLE IF EXISTS " + table_name + ";\n"};

		int rc = sqlite3_open(cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			log::error("drop_package_database.sqlite3_open(): {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return rc;
		}
		
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			log::error("drop_package_database.sqlite3_exec(): {}", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return rc;
		}

		return 0;
	}

	std::string to_values(const package_info& p){
		std::string p_values{};
		std::string p_title = p.title; /* need copy for utils::replace_all() */

		p_values += fmt::to_string(p.asset_id) + ", ";
		p_values += "'" + p.type + "', ";
		p_values += "'" + utils::replace_all(p_title, "'", "''") + "', ";
		p_values += "'" + p.author + "', ";
		p_values += fmt::to_string(p.author_id) + ", ";
		p_values += "'" + p.version + "', ";
		p_values += "'" + p.godot_version + "', ";
		p_values += "'" + p.cost + "', ";
		p_values += "'" + p.description + "', ";
		p_values += "'" + p.modify_date + "', ";
		p_values += "'" + p.support_level + "', ";
		p_values += "'" + p.category + "', ";
		p_values += "'" + p.remote_source + "', ";
		p_values += "'" + p.download_url + "', ";
		p_values += "'" + p.download_hash + "', ";
		p_values += fmt::to_string(p.is_installed) + ", ";
		p_values += "'" + p.install_path + "'";
		return p_values;
	}

	std::string to_values(const std::vector<package_info>& packages){
		std::string o;
		for(const auto& p : packages)
			o += to_values(p);
		return o;
	}

}