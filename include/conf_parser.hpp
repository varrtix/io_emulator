#pragma once

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rapidxml.hpp>

namespace conf {
class basic_parser {
public:
  using ptr = std::unique_ptr<basic_parser>;

  basic_parser() = delete;
  basic_parser(const basic_parser &) = delete;
  basic_parser &operator=(const basic_parser &) = delete;
  basic_parser(basic_parser &&) noexcept = default;
  basic_parser &operator=(basic_parser &&) noexcept = default;
  virtual ~basic_parser() = default;

  explicit basic_parser(const std::filesystem::path &filepath)
      : filepath_(filepath) {
    reload();
  }
  explicit basic_parser(std::filesystem::path &&filepath)
      : filepath_(std::move(filepath)) {
    reload();
  }

  virtual void reload(const std::filesystem::path &filepath = {});

protected:
  using xmldoc_ptr = std::unique_ptr<rapidxml::xml_document<>>;
  using xmlbuffer = std::vector<char>;

  std::filesystem::path filepath_;
  xmlbuffer buffer_;
  xmldoc_ptr xmldoc_;

  static xmlbuffer make_buffer(const std::filesystem::path &filepath);

  static xmldoc_ptr make_xmldoc(xmlbuffer &buffer) {
    auto xmldoc = std::make_unique<xmldoc_ptr::element_type>();
    xmldoc->parse<0>(buffer.data());
    return xmldoc;
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
    if (filepath_.empty())
      throw std::runtime_error("invalid filepath: " + filepath.string());
    else
      path = filepath_;
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

    static std::int32_t parse_driver_id(const node_type *node) {
      const auto &n = node_get_attr(node, "drv");
      return n.empty() ? -1 : std::stoi(n);
    }
  };

  explicit io_parser(const std::string &env_key)
      : basic_parser([&env_key]() {
          const auto value = std::getenv(env_key.c_str());
          if (value == nullptr)
            throw std::runtime_error("env variable '" + env_key +
                                     "' is not set");
          return value;
        }()) {
    reload();
  }

  static io_parser::ptr make_unique(const std::string &env_key) {
    return std::make_unique<io_parser>(env_key);
  }

  static io_parser::shared_ptr make_shared(const std::string &env_key) {
    return std::make_shared<io_parser>(env_key);
  }

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

  static std::string node_get_attr(const node_type *node,
                                   const std::string &name) {
    if (auto attr = node->first_attribute(name.c_str());
        attr != nullptr && attr->value()[0] != '\0')
      return attr->value();
    return "";
  }
};

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
} // namespace conf
