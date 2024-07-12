#include <iostream>

#include "conf_parser.hpp"
#include "terminal.hpp"

#include "commands_impl.hpp"

constexpr auto term_name = "iotest";

int main(int argc, char const *argv[]) {
  try {
    auto &term = termctl::terminal::shared();
    auto xmlparser = conf::io_parser::make_shared("IOXML_CONF_PATH");
    auto cmds = termctl::commands::make_vec(
        termctl::make_help_command(), termctl::make_exit_command(),
        termctl::make_get_command(xmlparser),
        termctl::make_set_command(xmlparser));
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
