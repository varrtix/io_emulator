#pragma once

#include <iostream>

#include "command.hpp"

namespace termctl {
inline basic_command::ptr make_exit_command() {
  return std::make_unique<basic_command>("exit", [](const auto &) {
    std::exit(EXIT_SUCCESS);
    return true;
  });
}

inline basic_command::ptr make_get_command(completion::items_type &&items) {
  return std::make_unique<basic_command>(
      "get",
      [](const auto &args) {
        auto arg = args.empty() ? "empty" : args[0];
        std::cout << "get param: [" << arg << "]" << std::endl;
        return true;
      },
      std::move(items));
}
} // namespace termctl
