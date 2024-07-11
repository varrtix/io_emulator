#pragma once

#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <variant>

#include <lb/drv_emu.hpp>

#include "command.hpp"
#include "conf_parser.hpp"

namespace ctf_io {
class variant final {
public:
  using raw_type = std::variant<int, double, std::string>;
  using item = conf::io_parser::item;

  template <typename T>
  static constexpr auto is_member_v =
      std::disjunction_v<std::is_same<T, int>, std::is_same<T, double>,
                         std::is_same<T, std::string>>;

  variant() = delete;
  variant(const variant &) = default;
  variant &operator=(const variant &) = default;
  variant(variant &&) noexcept = default;
  variant &operator=(variant &&) noexcept = default;
  ~variant() = default;

  friend std::ostream &operator<<(std::ostream &, const variant &);

  explicit variant(item::data_type type, const std::string &param) {
    reset(type, param);
  }

  void reset(item::data_type type, const std::string &param = {});
  auto &get() noexcept { return var_; }
  const auto &get() const noexcept { return var_; }

  bool fetch() {
    auto ret = IO_UNKNOWN_TYPE;
    if (auto ptr = std::get_if<int>(&var_))
      ret = io_read_int(param_.c_str(), ptr);
    else if (auto ptr = std::get_if<double>(&var_))
      ret = io_read_double(param_.c_str(), ptr);
    else if (auto ptr = std::get_if<std::string>(&var_))
      ret = io_read_string(param_, *ptr);
    return ret == IO_SUCCESS;
  }

  // template <typename T, std::enable_if_t<is_member_v<T>>>
  // auto operator*() -> typename std::add_lvalue_reference<T>::type {
  //   if (!std::holds_alternative<T>(var_))
  //     throw std::bad_variant_access{};
  //   return std::get<T>(var_);
  // }

  // template <typename T, std::enable_if_t<is_member_v<T>>>
  // friend variant &operator>>(variant &var, T &out);

  // template <typename T, std::enable_if_t<is_member_v<T>>>
  // bool read(T &out) noexcept {}

private:
  raw_type var_;
  std::string param_;

  template <typename T> static constexpr bool is_supported_type() {
    return std::is_same_v<T, int> || std::is_same_v<T, float> ||
           std::is_same_v<T, double> || std::is_same_v<T, std::string>;
  }
};

inline void variant::reset(item::data_type type, const std::string &param) {
  switch (type) {
  case item::data_type::int_val:
    var_ = 0;
  case item::data_type::double_val:
    var_ = 0.0;
  case item::data_type::string_val:
    var_ = std::string{};
  default:
    throw std::invalid_argument("unsupported type for constructor");
  }

  if (!param.empty())
    param_ = param;
}

inline bool perform_command_get(const termctl::basic_command::exec_args &args,
                                const conf::io_parser::shared_ptr &parser) {
  if (!args.empty())
    if (auto item = parser->find_item(args[0]); item && !item->pr.empty()) {
      auto val = variant(item->dt, item->pr);
      if (val.fetch()) {
        std::cout << "[OK][" << item->name << "][" << item->pr
                  << "] read: " << val << std::endl;
        return true;
      } else {
        std::cerr << "[FAIL][" << item->name << "][" << item->pr
                  << "] could not read, please write the value" << std::endl;
        return false;
      }
    }

  std::cerr << "failed to perform command 'get'" << std::endl;
  return false;
}

inline std::ostream &operator<<(std::ostream &os, const variant &v) {
  std::visit([&os](const auto &val) { os << val; }, v.get());
  return os;
}
} // namespace ctf_io

namespace termctl {
inline basic_command::ptr make_exit_command() {
  return std::make_unique<basic_command>("exit", [](const auto &) {
    std::exit(EXIT_SUCCESS);
    return true;
  });
}

inline basic_command::ptr
make_get_command(const completion::items_type &items) {
  return std::make_unique<basic_command>(
      "get",
      [](const auto &args) {
        auto arg = args.empty() ? "empty" : args[0];
        std::cout << "get param: [" << arg << "]" << std::endl;
        return true;
      },
      items);
}

inline basic_command::ptr make_get_command(conf::io_parser::shared_ptr parser) {
  return std::make_unique<basic_command>(
      "get",
      std::bind(ctf_io::perform_command_get, std::placeholders::_1, parser),
      parser->item_keys());
}

inline basic_command::ptr
make_set_command(const completion::items_type &items) {
  return std::make_unique<basic_command>(
      "set", [](const auto &args) { return true; }, items);
}
} // namespace termctl
