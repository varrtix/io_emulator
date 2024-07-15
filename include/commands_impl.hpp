#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

#include <lb/drv_emu.hpp>

#include "command.hpp"
#include "conf_parser.hpp"
#include "ctf_io.h"

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

  bool read();
  bool write(const std::string &val) noexcept;

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
    break;
  case item::data_type::double_val:
    var_ = 0.0;
    break;
  case item::data_type::string_val:
    var_ = std::string{};
    break;
  default:
    throw std::invalid_argument("unsupported type for reset variant");
  }

  if (!param.empty())
    std::visit(
        [&param, this](auto &&v) {
          using Tv = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<Tv, std::string>)
            param_ = lb::drv_emulator<char>{}.to_io_name(param.c_str());
          else
            param_ = lb::drv_emulator<Tv>{}.to_io_name(param.c_str());
        },
        var_);
}

inline bool variant::read() {
  auto ret = IO_UNKNOWN_TYPE;
  if (auto ptr = std::get_if<int>(&var_))
    ret = io_read_int(param_.c_str(), ptr);
  else if (auto ptr = std::get_if<double>(&var_))
    ret = io_read_double(param_.c_str(), ptr);
  else if (auto ptr = std::get_if<std::string>(&var_))
    ret = io_read_string(param_, *ptr);
  return ret == IO_SUCCESS;
}

inline bool variant::write(const std::string &val) noexcept {
  try {
    auto ret = IO_UNKNOWN_TYPE;
    if (std::holds_alternative<int>(var_))
      ret = io_write_int(param_.c_str(), std::stoi(val));
    else if (std::holds_alternative<double>(var_))
      ret = io_write_double(param_.c_str(), std::stod(val));
    else if (std::holds_alternative<std::string>(var_))
      ret = io_write_string(param_.c_str(), val.c_str());
    return ret == IO_SUCCESS;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return false;
}

inline bool perform_command_get(const termctl::basic_command::exec_args &args,
                                const conf::io_parser::shared_ptr &parser) {
  if (args.empty()) {
    std::cerr << "Error: requires exactly one argument on command" << std::endl;
  } else {
    auto item = parser->find_item(args[0]);
    if (!item) {
      std::cerr << "Error: invalid item of module " << std::quoted(args[0])
                << std::endl;
    } else if (item->pr.empty()) {
      std::cerr << "Error: the 'pr' value for module " << std::quoted(args[0])
                << " could not be empty" << std::endl;
    } else {
      auto val = variant(item->dt, item->pr);
      if (val.read()) {
        std::cout << "[OK][" << item->name << "][" << item->pr
                  << "] read: " << val << std::endl;
        return true;
      }

      std::cerr << "[FAIL][" << item->name << "][" << item->pr
                << "] could not be read, please might need to set a value first"
                << std::endl;
    }
  }

  return false;
}

inline bool perform_command_set(const termctl::basic_command::exec_args &args,
                                const conf::io_parser::shared_ptr &parser) {
  if (args.size() != 2) {
    std::cerr << "Error: requires exactly two arguments on command"
              << std::endl;
  } else {
    if (auto item = parser->find_item(args[0]); item) {
      auto prw = item->pw;
      if (prw.empty()) {
        std::cerr << "Warning: the value 'pw' is empty, attempting to use 'pr' "
                     "value ..."
                  << std::endl;

        prw = item->pr;
        if (prw.empty()) {
          std::cerr << "Error: the value 'pr' is empty, attempt to use 'pr' "
                       "value failed"
                    << std::endl;
          return false;
        }
      }

      auto val = variant(item->dt, prw);
      if (val.write(args[1])) {
        std::cout << "[OK][" << item->name << "][" << prw << "] write: " << val
                  << std::endl;
        return true;
      }

      std::cerr << "[FAIL][" << item->name << "][" << prw
                << "] failed to write: " << args[1] << std::endl;
    } else {
      std::cerr << "Error: invalid item of module " << std::quoted(args[0])
                << std::endl;
    }
  }

  return false;
}

inline bool perform_command_info(const termctl::basic_command::exec_args &args,
                                 const conf::io_parser::shared_ptr &parser) {
  if (args.empty()) {
    std::cerr << "Error: requires exactly one argument on command" << std::endl;
  } else {
    if (args[0] == "all") {
      for (const auto &item : parser->items())
        item.second.pretty_print();
      return true;
    } else if (auto item = parser->find_item(args[0]); item) {
      item->pretty_print();
      return true;
    }

    std::cerr << "Error: invalid item of module " << std::quoted(args[0])
              << std::endl;
  }

  return false;
}

inline std::ostream &operator<<(std::ostream &os, const variant &v) {
  std::visit(
      [&os](auto &&val) {
        using Tv = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<Tv, std::string>)
          os << val;
        else
          os << std::to_string(val);
      },
      v.get());
  return os;
}
} // namespace ctf_io

namespace termctl {
inline basic_command::ptr make_help_command() {
  return std::make_unique<basic_command>("help", [](const auto &) {
    std::cout << "Available commands:\n";
    std::cout << "  get  <module>                get the value of <module>\n";
    std::cout << "  set  <module>|pr/pw <value>  set <module> or <pr/pw> to "
                 "<value>\n";
    std::cout << "  info <module>|all            get the information of "
                 "<module>\n";
    std::cout << "  help                         display help text\n";
    std::cout << "  exit                         quit\n";
    return true;
  });
}

inline basic_command::ptr make_exit_command() {
  return std::make_unique<basic_command>("exit", [](const auto &) {
    std::exit(EXIT_SUCCESS);
    return true;
  });
}

inline basic_command::ptr make_get_command(conf::io_parser::shared_ptr parser) {
  return std::make_unique<basic_command>(
      "get",
      std::bind(ctf_io::perform_command_get, std::placeholders::_1, parser),
      parser->item_keys());
}

inline basic_command::ptr make_set_command(conf::io_parser::shared_ptr parser) {
  return std::make_unique<basic_command>(
      "set",
      std::bind(ctf_io::perform_command_set, std::placeholders::_1, parser),
      parser->item_keys());
}

inline basic_command::ptr
make_info_command(conf::io_parser::shared_ptr parser) {
  return std::make_unique<basic_command>(
      "info",
      std::bind(ctf_io::perform_command_info, std::placeholders::_1, parser),
      parser->item_keys());
}
} // namespace termctl
