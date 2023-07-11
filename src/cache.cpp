
#include "cache.hpp"
#include "error.hpp"
#include "log.hpp"
#include "constants.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "result.hpp"
#include <filesystem>
#include <string>
#include <format>
#include <tuple>


namespace gdpm::cache{
	std::string _escape_sql(const std::string& s){
		return utils::replace_all(s, "'", "''");
	}

	error create_package_database(
		bool overwrite, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg;

		/* Check and make sure directory is created before attempting to open */
		namespace fs = std::filesystem;
		fs::path dir_path = fs::path(params.cache_path).parent_path();
		if(!fs::exists(dir_path)){
			log::debug("Creating cache directories...{}", params.cache_path);
			fs::create_directories(dir_path);
		}

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR,
				std::format(
					"cache::create_package_database::sqlite3_open(): {}",
					sqlite3_errmsg(db)
				)
			);
			sqlite3_close(db);
			return error;
		}

		string sql = "CREATE TABLE IF NOT EXISTS " +
							params.table_name + "("
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
			error error(ec::SQLITE_ERR, std::format(
				"cache::create_package_database::sqlite3_exec(): {}", 
				errmsg
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return error;
		}
		sqlite3_close(db);
		return error();
	}


	error insert_package_info(
		const package::info_list& packages, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;

		/* Prepare values to use in sql statement */
		string sql{"BEGIN TRANSACTION; "};
		for(const auto& p : packages){
			sql += "INSERT INTO " + params.table_name + " (" GDPM_PACKAGE_CACHE_COLNAMES ") ";
			sql += "VALUES (" + to_values(p).unwrap_unsafe() + "); ";
		}
		sql += "COMMIT;";
		// log::println("{}", sql);
		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::insert_package_info::sqlite3_open(): {}", 
				sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return error;
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			error error = log::error_rc(ec::SQLITE_ERR,
				std::format("cache::insert_package_info::sqlite3_exec(): {}", errmsg)
			);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return error;
		}
		sqlite3_close(db);
		return error();
	}


	result_t<package::info_list> get_package_info_by_id(
		const package::id_list& package_ids, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		size_t p_size = 0;
		package::info_list p_vector;
		string sql{"BEGIN TRANSACTION;\n"};

		auto callback = [](void *data, int argc, char **argv, char **colnames){
			package::info_list *_p_vector = (package::info_list*) data;
			package::info p{
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

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::get_package_info_by_id::sqlite3_open(): {}", sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return result_t(package::info_list(), error);
		}
		
		for(const auto& p_id : package_ids){
			sql += "SELECT * FROM " + params.table_name + " WHERE asset_id=" + std::to_string(p_id)+ ";\n";
		}
		sql += "COMMIT;\n";
		rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&p_vector, &errmsg);
		if(rc != SQLITE_OK){
			error error(constants::error::SQLITE_ERR, std::format(
				"cache::get_package_info_by_id::sqlite3_exec(): {}", errmsg
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return result_t(package::info_list(), error);
		}
		sqlite3_close(db);
		return result_t(p_vector, error());
	}


	result_t<package::info_list> get_package_info_by_title(
		const package::title_list& package_titles, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		package::info_list p_vector;

		auto callback = [](void *data, int argc, char **argv, char **colnames){
			if(argc <= 0)
				return 1;
			package::info_list *_p_vector = (package::info_list*)data;
			// log::println("get_package_info_by_title.callback.argv: \n\t{}\n\t{}\n\t{}\n\t{}\n\t{}", argv[0], argv[1], argv[2],argv[3], argv[4]);
			package::info p{
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
		if(!std::filesystem::exists(params.cache_path))
			std::filesystem::create_directories(params.cache_path);

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::get_package_info_by_title::sqlite3_open(): {}", sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return result_t(package::info_list(), error);
		}

		string sql{"BEGIN TRANSACTION;"};
		for(const auto& p_title : package_titles){
			sql += "SELECT * FROM " + params.table_name + " WHERE title='" + p_title + "';";
		}
		sql += "COMMIT;";
		// log::println(sql);
		rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&p_vector, &errmsg);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::get_package_info_by_title::sqlite3_exec(): {}", errmsg
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return result_t(package::info_list(), error);
		}
		sqlite3_close(db);
		return result_t(p_vector, error());
	}


	result_t<package::info_list> get_installed_packages(const params& params){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		package::info_list p_vector;
		string sql{"BEGIN TRANSACTION;"};

		auto callback = [](void *data, int argc, char **argv, char **colnames){
			package::info_list *_p_vector = (package::info_list*) data;
			package::info p{
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

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::get_installed_packages::sqlite3_open(): {}", sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return result_t(package::info_list(), error);
		}

		sql += "SELECT * FROM " + params.table_name + " WHERE is_installed=1; COMMIT;";
		rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&p_vector, &errmsg);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::get_installed_packages::sqlite3_exec(): {}", errmsg
			));
			log::error(error);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return result_t(package::info_list(), error);
		}
		sqlite3_close(db);
		return result_t(p_vector, error());
	}


	error update_package_info(
		const package::info_list& packages, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		
		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(
				ec::SQLITE_ERR, std::format(
				"cache::update_package_info::sqlite3_open(): {}", sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return error;
		}
		
		string sql;
		for(const auto& p : packages){
			sql += "UPDATE " 	+ params.table_name + " SET "
			" asset_id=" 		+ std::to_string(p.asset_id) + ", "
			" type='" 			+ _escape_sql(p.type) + "', "
			" title='" 			+ _escape_sql(p.title) + "', "
			" author='" 		+ _escape_sql(p.author) + "', " + 
			" author_id=" 		+ std::to_string(p.author_id) + ", "
			" version='" 		+ _escape_sql(p.version) + "', " + 
			" godot_version='" 	+ _escape_sql(p.godot_version) + "', " +
			" cost='" 			+ _escape_sql(p.cost) + "', " +
			" description='" 	+ _escape_sql(p.description) + "', " +
			" modify_date='" 	+ _escape_sql(p.modify_date) + "', " +
			" support_level='" 	+ _escape_sql(p.support_level) + "', " +
			" category='" 		+ _escape_sql(p.category) + "', " +
			" remote_source='" 	+ _escape_sql(p.remote_source) + "', " +
			" download_url='" 	+ _escape_sql(p.download_url) + "', " + 
			" download_hash='" 	+ _escape_sql(p.download_hash) + "', " +
			" is_installed=" 	+ std::to_string(p.is_installed) + ", " 
			" install_path='" 	+ _escape_sql(p.install_path) + "'"
			// " dependencies='" + p.dependencies + "'"
			" WHERE title='" 	+ p.title + "' AND asset_id=" + std::to_string(p.asset_id)
			+ ";\n";
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::update_package_info::sqlite3_exec(): {}\n\t{}", errmsg, sql
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return error;
		}
		sqlite3_close(db);
		return error();
	}


	error delete_packages(
		const package::title_list& package_titles, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		string sql;

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::delete_packages::sqlite3_open(): {}", sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return error;
		}

		for(const auto& p_title : package_titles){
			sql += "DELETE FROM " + params.table_name + " WHERE title='"
			+ p_title + "';\n";
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::delete_packages::sqlite3_exec(): {}", errmsg
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return error;
		}
		sqlite3_close(db);
		return error();
	}


	error delete_packages(
		const package::id_list& package_ids, 
		const params& params
	){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		string sql;

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::delete_packages::sqlite3_open(): {}", errmsg
			));
			sqlite3_close(db);
			return error;
		}

		for(const auto& p_id : package_ids){
			sql += "DELETE FROM " + params.table_name + " WHERE asset_id="
			+ std::to_string(p_id) + ";\n";
		}
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::delete_packages::sqlite3_exec(): {}", errmsg
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return error;
		}
		sqlite3_close(db);
		return error();
	}


	error drop_package_database(const params& params){
		sqlite3 *db;
		sqlite3_stmt *res;
		char *errmsg = nullptr;
		string sql{"DROP TABLE IF EXISTS " + params.table_name + ";\n"};

		int rc = sqlite3_open(params.cache_path.c_str(), &db);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::drop_package_database::sqlite3_open(): {}", sqlite3_errmsg(db)
			));
			sqlite3_close(db);
			return error;
		}
		
		rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
		if(rc != SQLITE_OK){
			error error(ec::SQLITE_ERR, std::format(
				"cache::drop_package_database::sqlite3_exec(): {}", errmsg
			));
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return error;
		}

		return error();
	}


	result_t<string> to_values(const package::info& p){
		string p_values{};
		string p_title = p.title; /* need copy for utils::replace_all() */

		p_values += std::to_string(p.asset_id) + ", ";
		p_values += "'" + p.type + "', ";
		p_values += "'" + utils::replace_all(p_title, "'", "''") + "', ";
		p_values += "'" + p.author + "', ";
		p_values += std::to_string(p.author_id) + ", ";
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
		p_values += std::to_string(p.is_installed) + ", ";
		p_values += "'" + p.install_path + "'";
		return result_t(p_values, error());
	}


	result_t<string> to_values(const package::info_list& packages){
		string o;
		for(const auto& p : packages)
			o += to_values(p).unwrap_unsafe();
		return result_t(o, error());
	}

}