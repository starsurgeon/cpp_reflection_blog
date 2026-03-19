#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <meta>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <type_traits>

namespace db_mapping
{

  template<typename>
  inline constexpr bool always_false_v = false;

  template<typename T>
  consteval auto reflected_members()
  {
    constexpr auto ctx = std::meta::access_context::current();
    return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));
  }

  template<typename T>
  consteval auto column_names()
  {
    constexpr auto members = reflected_members<T>();
    std::array<std::string_view, members.size()> columns{};

    for (std::size_t i = 0; i < members.size(); ++i)
    {
      columns[i] = std::meta::identifier_of(members[i]);
    }
    return columns;
  }

  template<typename T>
  consteval auto table_name() -> std::string_view
  {
    return std::meta::identifier_of(^^T);
  }

  template<typename T>
  auto insert_sql() -> std::string
  {
    constexpr auto cols = column_names<T>();
    std::string sql = "INSERT INTO ";
    sql += table_name<T>();
    sql += " (";

    for (std::size_t i = 0; i < cols.size(); ++i)
    {
      if (i != 0)
      {
        sql += ", ";
      }
      sql += cols[i];
    }

    sql += ") VALUES (";
    for (std::size_t i = 0; i < cols.size(); ++i)
    {
      if (i != 0)
      {
        sql += ", ";
      }
      sql += "?";
    }
    sql += ")";

    return sql;
  }

  template<typename T>
  auto select_sql() -> std::string
  {
    constexpr auto cols = column_names<T>();
    std::string sql = "SELECT ";

    for (std::size_t i = 0; i < cols.size(); ++i)
    {
      if (i != 0)
      {
        sql += ", ";
      }
      sql += cols[i];
    }

    sql += " FROM ";
    sql += table_name<T>();
    return sql;
  }

  template<typename Value>
  int bind_value(sqlite3_stmt *stmt, int index, Value const &value)
  {
    using Clean = std::remove_cvref_t<Value>;

    if constexpr (std::is_same_v<Clean, int>)
    {
      return sqlite3_bind_int(stmt, index, value);
    }
    else if constexpr (std::is_same_v<Clean, std::string>)
    {
      return sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
    }
    else if constexpr (std::is_same_v<Clean, std::string_view>)
    {
      return sqlite3_bind_text(stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
    }
    else
    {
      static_assert(always_false_v<Clean>, "Unsupported type for sqlite binding");
    }
  }

  template<typename Value>
  int read_value(sqlite3_stmt *stmt, int column, Value &value)
  {
    using Clean = std::remove_cvref_t<Value>;

    if constexpr (std::is_same_v<Clean, int>)
    {
      value = sqlite3_column_int(stmt, column);
      return SQLITE_OK;
    }
    else if constexpr (std::is_same_v<Clean, std::string>)
    {
      auto const *text = sqlite3_column_text(stmt, column);
      if (text == nullptr)
      {
        value.clear();
      }
      else
      {
        int const size = sqlite3_column_bytes(stmt, column);
        value.resize(static_cast<std::size_t>(size));
        std::memcpy(value.data(), text, static_cast<std::size_t>(size));
      }
      return SQLITE_OK;
    }
    else
    {
      static_assert(always_false_v<Clean>, "Unsupported type for sqlite extraction");
    }
  }

  template<typename T>
  int bind_all(sqlite3_stmt *stmt, T const &object)
  {
    int index = 1;
    int rc = SQLITE_OK;

    template for (constexpr auto member : reflected_members<T>())
    {
      if (rc == SQLITE_OK)
      {
        rc = bind_value(stmt, index++, object.[:member:]);
      }
    }

    return rc;
  }

  template<typename T>
  int extract_row(sqlite3_stmt *stmt, T &object)
  {
    int column = 0;
    int rc = SQLITE_OK;

    template for (constexpr auto member : reflected_members<T>())
    {
      if (rc == SQLITE_OK)
      {
        rc = read_value(stmt, column++, object.[:member:]);
      }
    }

    return rc;
  }

} // namespace db_mapping
