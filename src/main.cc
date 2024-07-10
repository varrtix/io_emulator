#include <iostream>

#include "conf_parser.hpp"
#include "terminal.hpp"

#include "commands_impl.hpp"

constexpr auto term_name = "iotest";

int main(int argc, char const *argv[]) {
  try {
    auto &term = termctl::terminal::shared();
    auto cmds = std::vector<termctl::basic_command::ptr>();
    auto xmlparser = conf::io_parser::make_unique("IOXML_CONF_PATH");
    auto get_param = xmlparser->item_keys();

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
