#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include <rapidxml.hpp>

namespace conf {
class basic_parser {
public:
  using ptr = std::unique_ptr<basic_parser>;

  basic_parser() = delete;
  basic_parser(basic_parser &&) noexcept = default;
  basic_parser &operator=(basic_parser &&) noexcept = default;
  virtual ~basic_parser() = default;

  explicit basic_parser(const std::filesystem::path &filepath)
      : filepath_(filepath), xmldoc_(make_xmldoc(filepath_, buffer_)) {}
  explicit basic_parser(std::filesystem::path &&filepath)
      : filepath_(std::move(filepath)),
        xmldoc_(make_xmldoc(filepath_, buffer_)) {}

protected:
  using xmldoc_ptr = std::unique_ptr<rapidxml::xml_document<>>;
  using xmlbuffer = std::vector<char>;

  std::filesystem::path filepath_;
  xmldoc_ptr xmldoc_;
  xmlbuffer buffer_;

private:
  inline static xmldoc_ptr make_xmldoc(const std::filesystem::path &filepath,
                                       xmlbuffer &buffer) {
    auto xmlfile = std::ifstream(filepath, std::ios::binary);
    auto xmldoc = std::make_unique<xmldoc_ptr::element_type>();
    if (!xmlfile.is_open())
      throw std::runtime_error("cannot open file: " + filepath.string());

    buffer = xmlbuffer((std::istreambuf_iterator<char>(xmlfile)),
                       std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    xmldoc->parse<0>(buffer.data());

    return xmldoc;
  }
};
} // namespace conf

namespace conf {
class io_parser final : public basic_parser {
public:
  using basic_parser::basic_parser;

  explicit io_parser(const std::string &env_key)
      : basic_parser([&env_key]() {
          const auto value = std::getenv(env_key.c_str());
          if (value == nullptr)
            throw std::runtime_error("env variable '" + env_key +
                                     "' is not set");
          return std::filesystem::path(value);
        }()) {}
};
} // namespace conf
