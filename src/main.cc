#include <iostream>
#include <string>

#ifdef CTF_CLI
#include <map>

#include <ctf_io.h>
#include <ctf_log.h>
#include <ctf_util.h>
#endif

#include "conf_parser.hpp"
#include "terminal.hpp"

#include "commands_impl.hpp"

constexpr auto term_name = "iotest";

int main(int argc, char *argv[]) {
  try {
    auto term_prompt = std::string(term_name);

#ifdef CTF_CLI
    auto args = std::map<std::string, std::string>();
    if (!ctf::CTFConsole::process_command_line(argc, argv, args)) {
      CTF_LOG(ctf::CL_ERROR, "failed to process command line: %s", argv[0]);
      return EXIT_FAILURE;
    }

    const auto module_name = ctf::CTFConsole::module_name();
    if (module_name == nullptr) {
      CTF_LOG(ctf::CL_ERROR, "failed to load module name");
      return EXIT_FAILURE;
    }

    term_prompt = module_name;
    set_log_module_name(module_name);
    if (ctf::CTFTask::check_running_flag(module_name)) {
      CTF_LOG(ctf::CL_ERROR, "[%s] is already running ...", module_name);
      ctf::CTFTask::exit_task_exp(module_name);
      return EXIT_FAILURE;
    }

    if (!ctf::ctf_load_proj_cfg()) {
      CTF_LOG(ctf::CL_ERROR, "[%s] failed to load configurations", module_name);
      ctf::CTFTask::exit_task_exp(module_name);
      return EXIT_FAILURE;
    }

    if (auto io_ret = io_init_client(); io_ret != IO_SUCCESS) {
      CTF_LOG(ctf::CL_ERROR,
              "[%s] failed to init the client of IO layer, code: %d",
              module_name, io_ret);
      ctf::ctf_release_proj_cfg();
      ctf::CTFTask::exit_task_exp(module_name);
      return EXIT_FAILURE;
    }
#endif

    auto &term = termctl::terminal::shared();
    auto xmlparser = conf::io_parser::make_shared("IOXML_CONF_PATH");
    auto cmds = termctl::commands::make_vec(
        termctl::make_help_command(), termctl::make_exit_command(),
        termctl::make_info_command(xmlparser),
        termctl::make_get_command(xmlparser),
        termctl::make_set_command(xmlparser));

    term.register_commands(std::move(cmds));

    term.run(term_prompt);

#ifdef CTF_CLI
    ctf::CTFTask::set_running_flag(module_name);
    ctf::CTFTask::wait_exit_signal(module_name);
    ctf::CTFTask::exit_task_exp(module_name);
#endif

    return EXIT_SUCCESS;

  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception caught in main." << std::endl;
  }

  return EXIT_FAILURE;
}
