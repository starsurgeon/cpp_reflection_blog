#include <iostream>
#include <sqlite3.h>
#include <string>

#include "sqlite_mapper.h"

namespace
{

  struct User
  {
    int id;
    std::string name;
    int age;
  };

  int exec_sql(sqlite3 *db, char const *sql)
  {
    char *err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK)
    {
      std::cerr << "sqlite3_exec failed: " << (err != nullptr ? err : "unknown error") << '\n';
      sqlite3_free(err);
    }
    return rc;
  }

  int prepare_stmt(sqlite3 *db, std::string const &sql, sqlite3_stmt **stmt)
  {
    int rc = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), stmt, nullptr);
    if (rc != SQLITE_OK)
    {
      std::cerr << "sqlite3_prepare_v2 failed: " << sqlite3_errmsg(db) << '\n';
    }
    return rc;
  }

} // namespace

int main()
{
  sqlite3 *db = nullptr;
  int rc = sqlite3_open(":memory:", &db);
  if (rc != SQLITE_OK)
  {
    std::cerr << "sqlite3_open failed\n";
    if (db != nullptr)
    {
      sqlite3_close(db);
    }
    return 1;
  }

  const auto insert = db_mapping::insert_sql<User>();
  const auto select = db_mapping::select_sql<User>();

  std::cout << "Generated INSERT SQL: " << insert << '\n';
  std::cout << "Generated SELECT SQL: " << select << '\n';

  rc = exec_sql(db, "CREATE TABLE User ("
                    "id INTEGER PRIMARY KEY, "
                    "name TEXT NOT NULL, "
                    "age INTEGER NOT NULL"
                    ");");
  if (rc != SQLITE_OK)
  {
    sqlite3_close(db);
    return 1;
  }

  sqlite3_stmt *insert_stmt = nullptr;
  rc = prepare_stmt(db, insert, &insert_stmt);
  if (rc != SQLITE_OK)
  {
    sqlite3_close(db);
    return 1;
  }

  User source{ 1, "Ada", 37 };
  rc = db_mapping::bind_all(insert_stmt, source);
  if (rc != SQLITE_OK)
  {
    std::cerr << "bind_all failed with code " << rc << '\n';
    sqlite3_finalize(insert_stmt);
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_step(insert_stmt);
  if (rc != SQLITE_DONE)
  {
    std::cerr << "insert step failed: " << sqlite3_errmsg(db) << '\n';
    sqlite3_finalize(insert_stmt);
    sqlite3_close(db);
    return 1;
  }
  sqlite3_finalize(insert_stmt);

  sqlite3_stmt *select_stmt = nullptr;
  rc = prepare_stmt(db, select, &select_stmt);
  if (rc != SQLITE_OK)
  {
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_step(select_stmt);
  if (rc != SQLITE_ROW)
  {
    std::cerr << "select step failed: " << sqlite3_errmsg(db) << '\n';
    sqlite3_finalize(select_stmt);
    sqlite3_close(db);
    return 1;
  }

  User loaded{};
  rc = db_mapping::extract_row(select_stmt, loaded);
  if (rc != SQLITE_OK)
  {
    std::cerr << "extract_row failed with code " << rc << '\n';
    sqlite3_finalize(select_stmt);
    sqlite3_close(db);
    return 1;
  }

  std::cout << "Loaded row => id=" << loaded.id << ", name=" << loaded.name << ", age=" << loaded.age << '\n';

  sqlite3_finalize(select_stmt);
  sqlite3_close(db);
  return 0;
}
