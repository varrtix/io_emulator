#pragma once

#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace termctl {
class basic_completion {
public:
  using ptr = std::unique_ptr<basic_completion>;
  using items_type = std::vector<std::string>;
  using generator_func = std::function<char *(const char *, int)>;

  basic_completion() = delete;
  basic_completion(basic_completion &&) noexcept = default;
  basic_completion &operator=(basic_completion &&) noexcept = default;
  virtual ~basic_completion() = default;

  explicit basic_completion(const items_type &items)
      : items_(items), iter_(items_.cend()) {}
  explicit basic_completion(items_type &&items) noexcept
      : items_(std::move(items)), iter_(items_.cend()) {}

  explicit operator bool() const noexcept { return !items_.empty(); }

  virtual char *generator(const char *text, int state) = 0;

protected:
  items_type items_;
  items_type::const_iterator iter_;
};

class completion final : public basic_completion {
public:
  using basic_completion::basic_completion;

  inline static ptr make_unique(items_type &&items) noexcept {
    return std::make_unique<completion>(std::move(items));
  }

  char *generator(const char *text, int state) override;
};

inline char *completion::generator(const char *text, int state) {
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

  return nullptr;
}
} // namespace termctl
