#include <iostream>

#include "terminal.hpp"

#include "commands_impl.hpp"

constexpr auto term_name = "iotest";

int main(int argc, char const *argv[]) {
  try {
    auto &term = termctl::terminal::shared();
    auto cmds = std::vector<termctl::basic_command::ptr>();
    auto &&get_param = std::vector<std::string>{
        "CM1", "CM2", "PM.HHP", "TCP", "#12.331.1", "#11.33.1",
    };

    cmds.push_back(termctl::make_exit_command());
    cmds.push_back(termctl::make_get_command(std::move(get_param)));
    term.register_commands(std::move(cmds));

    term.run(term_name);

    return EXIT_SUCCESS;

  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception caught in main." << std::endl;
  }

  return EXIT_FAILURE;
}
