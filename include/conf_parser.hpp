#pragma once

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rapidxml.hpp>

#include <ctf_util.h>

#include "db.hpp"

namespace conf {
class basic_parser {
public:
  using ptr = std::unique_ptr<basic_parser>;

  basic_parser(const basic_parser &) = delete;
  basic_parser &operator=(const basic_parser &) = delete;
  basic_parser(basic_parser &&) noexcept = default;
  basic_parser &operator=(basic_parser &&) noexcept = default;
  virtual ~basic_parser() = default;

  explicit basic_parser(const std::filesystem::path &filepath) {
    reload(filepath);
  }
  explicit basic_parser(std::filesystem::path &&filepath)
      : filepath_(std::move(filepath)) {
    reload();
  }

  virtual void reload(const std::filesystem::path &filepath = {});

  bool good() const { return valid_filepath(filepath_); }

protected:
  using xmldoc_ptr = std::unique_ptr<rapidxml::xml_document<>>;
  using xmlbuffer = std::vector<char>;

  basic_parser() = default;

  std::filesystem::path filepath_;
  xmlbuffer buffer_;
  xmldoc_ptr xmldoc_;

  static xmlbuffer make_buffer(const std::filesystem::path &filepath);

  static xmldoc_ptr make_xmldoc(xmlbuffer &buffer) {
    auto xmldoc = std::make_unique<xmldoc_ptr::element_type>();
    xmldoc->parse<0>(buffer.data());
    return xmldoc;
  }

  static bool valid_filepath(const std::filesystem::path &filepath) {
    return std::filesystem::is_regular_file(filepath);
  }
};

inline basic_parser::xmlbuffer
basic_parser::make_buffer(const std::filesystem::path &filepath) {
  auto xmlifs = std::ifstream(filepath, std::ios::binary);
  if (!xmlifs.is_open() || xmlifs.bad())
    throw std::runtime_error("cannot open file: " + filepath.string());

  xmlifs.seekg(0, std::ios::end);
  const auto size = std::streamsize(xmlifs.tellg());
  xmlifs.seekg(0, std::ios::beg);
  if (size <= 0)
    throw std::runtime_error("bad file: " + filepath.string());

  auto buffer = xmlbuffer(size + 1);
  if (xmlifs.read(buffer.data(), size))
    buffer[size] = '\0';
  else
    throw std::runtime_error("cannot read file: " + filepath.string());

  xmlifs.close();

  return buffer;
}

inline void basic_parser::reload(const std::filesystem::path &filepath) {
  auto path = filepath;
  if (path.empty()) {
    if (valid_filepath(filepath_))
      path = filepath_;
    else
      throw std::runtime_error("bad filepath");

  } else if (!valid_filepath(path)) {
    throw std::runtime_error("invalid filepath: " + path.string());
  }

  buffer_ = make_buffer(path);
  xmldoc_ = make_xmldoc(buffer_);
  if (path == filepath)
    filepath_ = std::move(path);
}
} // namespace conf

namespace conf {
class io_parser final : public basic_parser {
  static constexpr auto root_node_k = "IODEF";
  static constexpr auto drvs_node_k = "DRIVERS";
  static constexpr auto items_node_k = "ITEMS";

  static constexpr auto drv_attr_k = "DRV";
  static constexpr auto item_attr_k = "ITEM";

  static constexpr auto ioconf_path_k = "IOXML_CONF_PATH";
  static constexpr auto ioconf_path_suffix = "workspace/conf/conf-io.xml";

  static constexpr auto db_path_k = "IODATA_PATH";
  static constexpr auto db_filename = "iodata";

public:
  struct driver;
  struct item;

  using basic_parser::basic_parser;
  using ptr = std::unique_ptr<io_parser>;
  using shared_ptr = std::shared_ptr<io_parser>;
  using node_type = rapidxml::xml_node<>;
  using drivers_type = std::unordered_map<std::uint32_t, driver>;
  using items_type = std::unordered_map<std::string, item>;
  using item_keys_type = std::unordered_set<std::string>;

  struct driver {
    std::int32_t id;
    std::int32_t node;
    std::string name;
    std::string file;
    bool enable;

    driver() = delete;
    ~driver() = default;
    driver(const driver &) = default;
    driver &operator=(const driver &) = default;
    driver(driver &&) noexcept = default;
    driver &operator=(driver &&) noexcept = default;

    explicit driver(const node_type *node)
        : id(std::stoi(node_get_attr(node, "id"))), node(parse_node_attr(node)),
          name(node_get_attr(node, "name")), file(node_get_attr(node, "file")),
          enable(node_get_attr(node, "enable") == "True") {}

  private:
    static std::int32_t parse_node_attr(const node_type *node) {
      const auto &n = node_get_attr(node, "node");
      return n.empty() ? -1 : std::stoi(n);
    }
  };

  struct item {
    enum class category_type { memory, io };
    enum class data_type {
      unknown,
      int_val,
      double_val,
      string_val,
    };

    category_type category;
    data_type dt;
    std::int32_t driver_id;
    std::string name;
    std::string pr;
    std::string pw;

    item() = delete;
    ~item() = default;
    item(const item &) = default;
    item &operator=(const item &) = default;
    item(item &&) noexcept = default;
    item &operator=(item &&) noexcept = default;

    explicit item(const node_type *node)
        : category(parse_category(node)), dt(parse_data_type(node)),
          driver_id(parse_driver_id(node)), name(node_get_attr(node, "name")),
          pr(node_get_attr(node, "pr")), pw(node_get_attr(node, "pw")) {}

    void pretty_print() const noexcept {
      std::stringstream ss;
      ss << "[name: " << name << "]";
      ss << "[dt: " << data_type_to_str(dt) << "]";
      ss << "[cat: " << (category == category_type::io ? "IO" : "Memory")
         << "]";
      ss << "[drv: " << (driver_id >= 0 ? std::to_string(driver_id) : "Nil")
         << "]";
      ss << "[pr: " << (pr.empty() ? "Nil" : pr) << "]";
      ss << "[pw: " << (pw.empty() ? "Nil" : pw) << "]";
      std::cout << ss.str() << std::endl;
    }

  private:
    static category_type parse_category(const node_type *node) {
      return node_get_attr(node, "cat") == "IO" ? category_type::io
                                                : category_type::memory;
    }

    static data_type parse_data_type(const node_type *node) {
      const auto &n = node_get_attr(node, "dt");
      if (n == "Integer")
        return data_type::int_val;
      else if (n == "Double")
        return data_type::double_val;
      else if (n == "String")
        return data_type::string_val;
      else
        return data_type::unknown;
    }

    static std::string data_type_to_str(const data_type type) noexcept {
      switch (type) {
      case data_type::int_val:
        return "Integer";
      case data_type::double_val:
        return "Double";
      case data_type::string_val:
        return "String";
      default:
        return "Nil";
      }
    }

    static std::int32_t parse_driver_id(const node_type *node) {
      const auto &n = node_get_attr(node, "drv");
      return n.empty() ? -1 : std::stoi(n);
    }
  };

  explicit io_parser(const std::string env_key = ioconf_path_k,
                     bool enable_persist = true);

  static io_parser::ptr make_unique() { return std::make_unique<io_parser>(); }

  static io_parser::shared_ptr make_shared() {
    return std::make_shared<io_parser>();
  }

  const items_type &items() const noexcept { return items_; }
  const item_keys_type &item_keys() const noexcept { return item_keys_; }

  std::optional<driver> find_driver(std::uint32_t id) const {
    if (auto it = drivers_.find(id); it != drivers_.cend())
      return it->second;
    return std::nullopt;
  }

  std::optional<item> find_item(const std::string &name) const {
    if (auto it = items_.find(name); it != items_.cend())
      return it->second;
    return std::nullopt;
  }

  void reload(const std::filesystem::path &filepath = {}) override;

private:
  drivers_type drivers_;
  items_type items_;
  item_keys_type item_keys_;

  bool enable_persist_;
  std::filesystem::path dbpath_;
  db::wrapper::ptr db_;

  static std::string node_get_attr(const node_type *node,
                                   const std::string &name) {
    if (auto attr = node->first_attribute(name.c_str());
        attr != nullptr && attr->value()[0] != '\0')
      return attr->value();
    return "";
  }

  bool reload_db();
};

inline io_parser::io_parser(const std::string env_key, bool enable_persist)
    : basic_parser(), enable_persist_(enable_persist) {
  const auto value = std::getenv(env_key.c_str());
  auto filepath = std::filesystem::path();
  if (value != nullptr) {
    filepath = value;
  } else if (auto root = ctf::get_current_eq_proj(); !root.empty()) {
    filepath = std::move(root);
    filepath /= ioconf_path_suffix;
  } else {
    std::cerr << "Warning: no config file could be load, please use "
                 "command 'load' to read first"
              << std::endl;
    return;
  }

  reload(filepath);
  reload_db();
}

inline void io_parser::reload(const std::filesystem::path &filepath) {
  basic_parser::reload(filepath);
  constexpr auto err_prefix = "failed to parse, node was not found: ";
  auto root_node = xmldoc_->first_node(root_node_k);
  if (root_node == nullptr)
    throw std::runtime_error(std::string(err_prefix) + root_node_k);

  auto drvs_node = root_node->first_node(drvs_node_k);
  if (drvs_node == nullptr)
    throw std::runtime_error(std::string(err_prefix) + drvs_node_k);

  auto items_node = drvs_node->next_sibling(items_node_k);
  if (items_node == nullptr)
    throw std::runtime_error(std::string(err_prefix) + items_node_k);

  auto drivers = drivers_type();
  for (auto node = drvs_node->first_node(drv_attr_k); node != nullptr;
       node = node->next_sibling())
    if (std::string_view(node->name()) == drv_attr_k) {
      auto drv = driver(node);
      drivers.emplace(drv.id, std::move(drv));
    }

  auto items = items_type();
  auto item_keys = item_keys_type();
  for (auto node = items_node->first_node(item_attr_k); node != nullptr;
       node = node->next_sibling())
    if (std::string_view(node->name()) == item_attr_k) {
      auto i = item(node);
      item_keys.emplace(i.name);
      items.emplace(i.name, std::move(i));
    }

  drivers_ = std::move(drivers);
  items_ = std::move(items);
  item_keys_ = std::move(item_keys);
}

inline bool io_parser::reload_db() {
  if (enable_persist_) {
    const auto value = std::getenv(db_path_k);
    auto dbpath = std::filesystem::path();
    if (value != nullptr)
      dbpath = value;
    else
      dbpath =
          (filepath_.has_parent_path() ? filepath_.parent_path() : filepath_) /
          db_filename;

    db_ = std::make_unique<db::wrapper>(dbpath);
    if (db_) {
      dbpath_ = std::move(dbpath);
      return true;
    }
  }
  return false;
}
} // namespace conf
