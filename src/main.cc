#include <iostream>

#include "command.hpp"
#include "completion.hpp"

constexpr auto term_name = "iotest";

inline void register_all_commands() {
  auto cmds = std::vector<termctl::basic_command::ptr>{
      termctl::make_exit_command(),
  };
  for (auto &cmd : cmds)
    termctl::commands::shared().register_command(std::move(cmd));
}

int main(int argc, char const *argv[]) {
  try {
    register_all_commands();
    auto cmd_completion = termctl::completion::make_unique(
        std::vector<std::string>{"help", "list", "exit"});

    // termctl::commands::shared().register();

    return EXIT_SUCCESS;

  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception caught in main." << std::endl;
  }

  return EXIT_FAILURE;
}
