#pragma once

#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <db_cxx.h>

namespace db {
class wrapper {
public:
  using ptr = std::unique_ptr<wrapper>;
  using key_type = std::string;

  wrapper(const wrapper &) = delete;
  wrapper &operator=(const wrapper &) = delete;
  wrapper(wrapper &&) noexcept = default;
  wrapper &operator=(wrapper &&) noexcept = default;

  virtual ~wrapper() {
    if (db_)
      db_->close(0);
  }

  explicit wrapper(const std::filesystem::path &dbpath)
      : db_(open_db(dbpath)) {}

  template <typename T> bool put(const key_type &key, T &&val);

  template <typename T,
            typename = std::enable_if_t<std::negation_v<std::is_const<T>>>>
  bool get(const key_type &key, T &val);

  bool remove(const key_type &key);

protected:
  using db_ptr = std::unique_ptr<Db>;
  using db_type = db_ptr::element_type;

  wrapper() = delete;

  db_ptr db_;

  static Dbt key_cast(const key_type &key) {
    return Dbt(const_cast<char *>(key.data()), key.size() + 1);
  }

private:
  static db_ptr open_db(const std::filesystem::path &dbpath) {
    if (dbpath.empty())
      throw std::invalid_argument("db path is empty");

    auto db = std::make_unique<db_type>(nullptr, 0);
    db->set_error_stream(&std::cerr);
    auto ret =
        db->open(nullptr, dbpath.c_str(), nullptr, DB_HASH, DB_CREATE, 0644);
    if (ret != 0)
      throw std::runtime_error(std::string("failed to open db: ") +
                               db_strerror(ret));

    return db;
  }
};

template <typename T> inline bool wrapper::put(const key_type &key, T &&val) {
  using value_type = std::decay_t<T>;
  auto k = key_cast(key);
  Dbt v(const_cast<value_type *>(std::addressof(val)), sizeof(value_type));
  return db_->put(nullptr, &k, &v, 0) == 0;
}

template <typename T, typename>
inline bool wrapper::get(const key_type &key, T &val) {
  auto k = key_cast(key);
  Dbt v;

  v.set_flags(DB_DBT_MALLOC);
  auto ret = db_->get(nullptr, std::addressof(k), std::addressof(v), 0);
  if (ret != DB_NOTFOUND) {
    if (v.get_size() == sizeof(T)) {
      std::memcpy(std::addressof(val), v.get_data(), sizeof(T));
      free(v.get_data());
      return true;
    }
    free(v.get_data());
  }

  return false;
}

inline bool wrapper::remove(const key_type &key) {
  auto k = key_cast(key);
  return db_->del(nullptr, &k, 0) == 0;
}
} // namespace db
