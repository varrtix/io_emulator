#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace termctl {
class basic_completion {
public:
  using ptr = std::unique_ptr<basic_completion>;
  using items_type = std::vector<std::string>;

  basic_completion() = delete;
  basic_completion(basic_completion &&) noexcept = default;
  basic_completion &operator=(basic_completion &&) noexcept = default;
  virtual ~basic_completion() = default;

  explicit basic_completion(const items_type &items);
  explicit basic_completion(items_type &&items) noexcept;

  virtual char *generator(const char *text, int state) noexcept = 0;

protected:
  items_type items_;
  items_type::const_iterator iter_;
};

inline basic_completion::basic_completion(const items_type &items)
    : items_(items), iter_(items_.cend()) {}
inline basic_completion::basic_completion(items_type &&items) noexcept
    : items_(std::move(items)), iter_(items_.cend()) {}

class completion final : public basic_completion {
public:
  using basic_completion::basic_completion;

  static ptr make_shared(items_type &&items) noexcept;

  char *generator(const char *text, int state) noexcept override;
};

inline char *completion::generator(const char *text, int state) noexcept {
  try {
    if (state == 0)
      iter_ = items_.cbegin();

    const auto len = std::strlen(text);
    if (len)
      while (iter_ != items_.cend()) {
        const auto &item = *iter_;
        ++iter_;
        if (item.compare(0, len, text) == 0)
          return strdup(item.c_str());
      }
  } catch (const std::exception &e) {
    std::cerr << "exception caught: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "unknown exception caught." << std::endl;
  }

  return nullptr;
}

inline completion::ptr
completion::make_shared(completion::items_type &&items) noexcept {
  return std::make_unique<completion>(std::move(items));
}
} // namespace termctl
