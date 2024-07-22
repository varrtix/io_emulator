#pragma once

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <readline/history.h>
#include <readline/readline.h>

#include "command.hpp"
#include "completion.hpp"

namespace termctl {
class terminal final {
public:
  terminal(const terminal &) = delete;
  terminal &operator=(const terminal &) = delete;

  static terminal &shared() {
    static terminal term_;
    return term_;
  }

  void set_prompt(const std::string &name) { prompt_ = name + "> "; }
  const std::string &prompt() const noexcept { return prompt_; }

  void register_commands(commands::vec_type &&cmds);

  void run(const std::string &name);
  void stop() noexcept { running_.store(false, std::memory_order_release); }

private:
  terminal() : running_(false) {
    rl_attempted_completion_function = command_completion;
  }

  static char **command_completion(const char *text, int start, int end);

  static char *generic_generator(const char *text, int state) {
    return shared().generator_ ? shared().generator_(text, state) : nullptr;
  }

  void assign_cmd_generator() noexcept {
    generator_ = std::bind(&basic_completion::generator, cmd_completion_.get(),
                           std::placeholders::_1, std::placeholders::_2);
  }

  void assign_param_generator(const std::string &name) {
    generator_ = cmds_.find_param_generator(name);
  }

  void reset_generator() { generator_ = nullptr; }

  std::string prompt_;
  commands cmds_;
  basic_completion::ptr cmd_completion_;
  basic_completion::generator_func generator_;
  std::atomic_bool running_;
};

inline char **terminal::command_completion(const char *text, int start,
                                           int end) {
  char **matches = nullptr;

  rl_attempted_completion_over = 1;
  shared().reset_generator();
  if (start == 0) {
    shared().assign_cmd_generator();
  } else {
    auto cmd = std::string_view(rl_line_buffer, start);
    auto pos = cmd.find(' ');
    if (pos != std::string::npos)
      cmd = cmd.substr(0, pos);
    shared().assign_param_generator(std::string(std::move(cmd)));
  }

  if (shared().generator_)
    matches = rl_completion_matches(text, generic_generator);

  return matches;
}

inline void terminal::register_commands(commands::vec_type &&cmds) {
  auto names = completion::items_type();
  for (auto &cmd : cmds) {
    names.emplace(cmd->get_name());
    cmds_.register_command(std::move(cmd));
  }

  cmd_completion_ = completion::make_unique(std::move(names));
}

inline void terminal::run(const std::string &name) {
  if (running_.load(std::memory_order_acquire)) {
    std::cerr << "Warning: terminal is already running ..." << std::endl;
    return;
  } else {
    running_.store(true, std::memory_order_release);
  }

  std::unique_ptr<char, decltype(&std::free)> input(nullptr, std::free);
  set_prompt(name);
  while (running_.load(std::memory_order_acquire)) {
    try {
      input.reset(readline(prompt().c_str()));
      if (input == nullptr || std::strlen(input.get()) == 0)
        throw std::invalid_argument("input is empty");

      std::istringstream iss(input.get());
      std::string cmd, arg;
      basic_command::exec_args args;
      if (!iss.good())
        throw std::invalid_argument("bad input");

      iss >> cmd;
      while (iss >> arg)
        args.push_back(std::move(arg));

      add_history(input.get());
      cmds_.execute_command(cmd, std::move(args));
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Error: unexpected error" << std::endl;
    }
  }
}
} // namespace termctl
