#pragma once

#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "completion.hpp"

namespace termctl {
class basic_command {
public:
  using ptr = std::unique_ptr<basic_command>;
  using exec_args = std::vector<std::string>;
  using execution = std::function<bool(const exec_args &)>;

  basic_command() = delete;
  basic_command(basic_command &&) noexcept = default;
  basic_command &operator=(basic_command &&) noexcept = default;
  virtual ~basic_command() = default;

  explicit basic_command(const std::string &name, execution &&exec,
                         completion::items_type &&items = {})
      : name_(name), exec_(std::move(exec)),
        param_completion_(items.empty()
                              ? nullptr
                              : completion::make_unique(std::move(items))) {}

  inline virtual char *param_generator(const char *text, int state) noexcept {
    return has_param() ? param_completion_->generator(text, state) : nullptr;
  }

  inline virtual bool execute(exec_args &&args) const {
    return exec_ ? exec_(args) : false;
  }

  inline const std::string &get_name() const noexcept { return name_; }
  inline bool has_param() const noexcept {
    return param_completion_ && *param_completion_;
  }

protected:
  std::string name_;
  execution exec_;
  completion::ptr param_completion_;
};

class commands final {
public:
  using map_type = std::unordered_map<std::string, basic_command::ptr>;

  commands() = default;
  commands(const commands &) = delete;
  commands &operator=(const commands &) = delete;
  commands(commands &&) noexcept = default;
  commands &operator=(commands &&) noexcept = default;
  ~commands() = default;

  inline void register_command(basic_command::ptr &&cmd) {
    cmds_map_.emplace(cmd->get_name(), std::move(cmd));
  }

  inline virtual bool execute_command(const std::string &name,
                                      basic_command::exec_args &&args) const {
    if (auto it = cmds_map_.find(name); it != cmds_map_.cend())
      return it->second->execute(std::move(args));
    return false;
  }

  inline basic_completion::generator_func
  find_param_generator(const std::string &name) const {
    if (auto it = cmds_map_.find(name); it != cmds_map_.cend())
      return std::bind(&basic_command::param_generator, it->second.get(),
                       std::placeholders::_1, std::placeholders::_2);
    return {};
  }

private:
  map_type cmds_map_;
};
} // namespace termctl
