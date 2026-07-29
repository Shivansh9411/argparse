/*
  __ _ _ __ __ _ _ __   __ _ _ __ ___  ___
 / _` | '__/ _` | '_ \ / _` | '__/ __|/ _ \ Argument Parser for Modern C++
| (_| | | | (_| | |_) | (_| | |  \__ \  __/ http://github.com/p-ranav/argparse
 \__,_|_|  \__, | .__/ \__,_|_|  |___/\___|
           |___/|_|

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2019-2022 Pranav Srinivas Kumar <pranav.srinivas.kumar@gmail.com>
and other contributors.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include <cerrno>

#ifndef ARGPARSE_MODULE_USE_STD_MODULE
#include <algorithm>
#include <any>
#include <array>
#include <set>
#include <charconv>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <filesystem>
#endif

#ifndef ARGPARSE_CUSTOM_STRTOF
#define ARGPARSE_CUSTOM_STRTOF strtof
#endif

#ifndef ARGPARSE_CUSTOM_STRTOD
#define ARGPARSE_CUSTOM_STRTOD strtod
#endif

#ifndef ARGPARSE_CUSTOM_STRTOLD
#define ARGPARSE_CUSTOM_STRTOLD strtold
#endif

namespace argparse {

namespace details { // namespace for helper methods

template <typename T, typename = void>
struct HasContainerTraits : std::false_type {};

template <> struct HasContainerTraits<std::string> : std::false_type {};

template <> struct HasContainerTraits<std::string_view> : std::false_type {};

template <typename T>
struct HasContainerTraits<
    T, std::void_t<typename T::value_type, decltype(std::declval<T>().begin()),
                   decltype(std::declval<T>().end()),
                   decltype(std::declval<T>().size())>> : std::true_type {};

template <typename T>
inline constexpr bool IsContainer = HasContainerTraits<T>::value;

template <typename T, typename = void>
struct HasStreamableTraits : std::false_type {};

template <typename T>
struct HasStreamableTraits<
    T,
    std::void_t<decltype(std::declval<std::ostream &>() << std::declval<T>())>>
    : std::true_type {};

template <typename T>
inline constexpr bool IsStreamable = HasStreamableTraits<T>::value;

constexpr std::size_t repr_max_container_size = 5;

template <typename T> std::string repr(T const &val) {
    __builtin_trap() /* STUB: not implemented */;
}

namespace {

template <typename T> constexpr bool standard_signed_integer = false;
template <> constexpr bool standard_signed_integer<signed char> = true;
template <> constexpr bool standard_signed_integer<short int> = true;
template <> constexpr bool standard_signed_integer<int> = true;
template <> constexpr bool standard_signed_integer<long int> = true;
template <> constexpr bool standard_signed_integer<long long int> = true;

template <typename T> constexpr bool standard_unsigned_integer = false;
template <> constexpr bool standard_unsigned_integer<unsigned char> = true;
template <> constexpr bool standard_unsigned_integer<unsigned short int> = true;
template <> constexpr bool standard_unsigned_integer<unsigned int> = true;
template <> constexpr bool standard_unsigned_integer<unsigned long int> = true;
template <>
constexpr bool standard_unsigned_integer<unsigned long long int> = true;

} // namespace

constexpr int radix_2 = 2;
constexpr int radix_8 = 8;
constexpr int radix_10 = 10;
constexpr int radix_16 = 16;

template <typename T>
constexpr bool standard_integer =
    standard_signed_integer<T> || standard_unsigned_integer<T>;

template <class F, class Tuple, class Extra, std::size_t... I>
constexpr decltype(auto)
apply_plus_one_impl(F &&f, Tuple &&t, Extra &&x,
                    std::index_sequence<I...> /*unused*/) {
  return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...,
                     std::forward<Extra>(x));
}

template <class F, class Tuple, class Extra>
constexpr decltype(auto) apply_plus_one(F &&f, Tuple &&t, Extra &&x) {
  return details::apply_plus_one_impl(
      std::forward<F>(f), std::forward<Tuple>(t), std::forward<Extra>(x),
      std::make_index_sequence<
          std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

constexpr auto pointer_range(std::string_view s) noexcept {
  return std::tuple(s.data(), s.data() + s.size());
}

template <class CharT, class Traits>
constexpr bool starts_with(std::basic_string_view<CharT, Traits> prefix,
                           std::basic_string_view<CharT, Traits> s) noexcept {
  return s.substr(0, prefix.size()) == prefix;
}

enum class chars_format {
  scientific = 0xf1,
  fixed = 0xf2,
  hex = 0xf4,
  binary = 0xf8,
  general = fixed | scientific
};

struct ConsumeBinaryPrefixResult {
  bool is_binary;
  std::string_view rest;
};

constexpr auto consume_binary_prefix(std::string_view s)
    -> ConsumeBinaryPrefixResult {
  if (starts_with(std::string_view{"0b"}, s) ||
      starts_with(std::string_view{"0B"}, s)) {
    s.remove_prefix(2);
    return {true, s};
  }
  return {false, s};
}

struct ConsumeHexPrefixResult {
  bool is_hexadecimal;
  std::string_view rest;
};

using namespace std::literals;

constexpr auto consume_hex_prefix(std::string_view s)
    -> ConsumeHexPrefixResult {
  if (starts_with("0x"sv, s) || starts_with("0X"sv, s)) {
    s.remove_prefix(2);
    return {true, s};
  }
  return {false, s};
}

template <class T, auto Param>
inline auto do_from_chars(std::string_view s) -> T {
    __builtin_trap() /* STUB: not implemented */;
}

template <class T, auto Param = 0> struct parse_number {
  auto operator()(std::string_view s) -> T {
    __builtin_trap() /* STUB: not implemented */;
}
};

template <class T> struct parse_number<T, radix_2> {
  auto operator()(std::string_view s) -> T {
    if (auto [ok, rest] = consume_binary_prefix(s); ok) {
      return do_from_chars<T, radix_2>(rest);
    }
    throw std::invalid_argument{"pattern not found"};
  }
};

template <class T> struct parse_number<T, radix_16> {
  auto operator()(std::string_view s) -> T {
    if (starts_with("0x"sv, s) || starts_with("0X"sv, s)) {
      if (auto [ok, rest] = consume_hex_prefix(s); ok) {
        try {
          return do_from_chars<T, radix_16>(rest);
        } catch (const std::invalid_argument &err) {
          throw std::invalid_argument("Failed to parse '" + std::string(s) +
                                      "' as hexadecimal: " + err.what());
        } catch (const std::range_error &err) {
          throw std::range_error("Failed to parse '" + std::string(s) +
                                 "' as hexadecimal: " + err.what());
        }
      }
    } else {
      // Allow passing hex numbers without prefix
      // Shape 'x' already has to be specified
      try {
        return do_from_chars<T, radix_16>(s);
      } catch (const std::invalid_argument &err) {
        throw std::invalid_argument("Failed to parse '" + std::string(s) +
                                    "' as hexadecimal: " + err.what());
      } catch (const std::range_error &err) {
        throw std::range_error("Failed to parse '" + std::string(s) +
                               "' as hexadecimal: " + err.what());
      }
    }

    throw std::invalid_argument{"pattern '" + std::string(s) +
                                "' not identified as hexadecimal"};
  }
};

template <class T> struct parse_number<T> {
  auto operator()(std::string_view s) -> T {
    auto [ok, rest] = consume_hex_prefix(s);
    if (ok) {
      try {
        return do_from_chars<T, radix_16>(rest);
      } catch (const std::invalid_argument &err) {
        throw std::invalid_argument("Failed to parse '" + std::string(s) +
                                    "' as hexadecimal: " + err.what());
      } catch (const std::range_error &err) {
        throw std::range_error("Failed to parse '" + std::string(s) +
                               "' as hexadecimal: " + err.what());
      }
    }

    auto [ok_binary, rest_binary] = consume_binary_prefix(s);
    if (ok_binary) {
      try {
        return do_from_chars<T, radix_2>(rest_binary);
      } catch (const std::invalid_argument &err) {
        throw std::invalid_argument("Failed to parse '" + std::string(s) +
                                    "' as binary: " + err.what());
      } catch (const std::range_error &err) {
        throw std::range_error("Failed to parse '" + std::string(s) +
                               "' as binary: " + err.what());
      }
    }

    if (starts_with("0"sv, s)) {
      try {
        return do_from_chars<T, radix_8>(rest);
      } catch (const std::invalid_argument &err) {
        throw std::invalid_argument("Failed to parse '" + std::string(s) +
                                    "' as octal: " + err.what());
      } catch (const std::range_error &err) {
        throw std::range_error("Failed to parse '" + std::string(s) +
                               "' as octal: " + err.what());
      }
    }

    try {
      return do_from_chars<T, radix_10>(rest);
    } catch (const std::invalid_argument &err) {
      throw std::invalid_argument("Failed to parse '" + std::string(s) +
                                  "' as decimal integer: " + err.what());
    } catch (const std::range_error &err) {
      throw std::range_error("Failed to parse '" + std::string(s) +
                             "' as decimal integer: " + err.what());
    }
  }
};

namespace {

template <class T> inline const auto generic_strtod = nullptr;
template <> inline const auto generic_strtod<float> = ARGPARSE_CUSTOM_STRTOF;
template <> inline const auto generic_strtod<double> = ARGPARSE_CUSTOM_STRTOD;
template <>
inline const auto generic_strtod<long double> = ARGPARSE_CUSTOM_STRTOLD;

} // namespace

template <class T> inline auto do_strtod(std::string const &s) -> T {
    __builtin_trap() /* STUB: not implemented */;
}

template <class T> struct parse_number<T, chars_format::general> {
  auto operator()(std::string const &s) -> T {
    if (auto r = consume_hex_prefix(s); r.is_hexadecimal) {
      throw std::invalid_argument{
          "chars_format::general does not parse hexfloat"};
    }
    if (auto r = consume_binary_prefix(s); r.is_binary) {
      throw std::invalid_argument{
          "chars_format::general does not parse binfloat"};
    }

    try {
      return do_strtod<T>(s);
    } catch (const std::invalid_argument &err) {
      throw std::invalid_argument("Failed to parse '" + s +
                                  "' as number: " + err.what());
    } catch (const std::range_error &err) {
      throw std::range_error("Failed to parse '" + s +
                             "' as number: " + err.what());
    }
  }
};

template <class T> struct parse_number<T, chars_format::hex> {
  auto operator()(std::string const &s) -> T {
    if (auto r = consume_hex_prefix(s); !r.is_hexadecimal) {
      throw std::invalid_argument{"chars_format::hex parses hexfloat"};
    }
    if (auto r = consume_binary_prefix(s); r.is_binary) {
      throw std::invalid_argument{"chars_format::hex does not parse binfloat"};
    }

    try {
      return do_strtod<T>(s);
    } catch (const std::invalid_argument &err) {
      throw std::invalid_argument("Failed to parse '" + s +
                                  "' as hexadecimal: " + err.what());
    } catch (const std::range_error &err) {
      throw std::range_error("Failed to parse '" + s +
                             "' as hexadecimal: " + err.what());
    }
  }
};

template <class T> struct parse_number<T, chars_format::binary> {
  auto operator()(std::string const &s) -> T {
    if (auto r = consume_hex_prefix(s); r.is_hexadecimal) {
      throw std::invalid_argument{
          "chars_format::binary does not parse hexfloat"};
    }
    if (auto r = consume_binary_prefix(s); !r.is_binary) {
      throw std::invalid_argument{"chars_format::binary parses binfloat"};
    }

    return do_strtod<T>(s);
  }
};

template <class T> struct parse_number<T, chars_format::scientific> {
  auto operator()(std::string const &s) -> T {
    if (auto r = consume_hex_prefix(s); r.is_hexadecimal) {
      throw std::invalid_argument{
          "chars_format::scientific does not parse hexfloat"};
    }
    if (auto r = consume_binary_prefix(s); r.is_binary) {
      throw std::invalid_argument{
          "chars_format::scientific does not parse binfloat"};
    }
    if (s.find_first_of("eE") == std::string::npos) {
      throw std::invalid_argument{
          "chars_format::scientific requires exponent part"};
    }

    try {
      return do_strtod<T>(s);
    } catch (const std::invalid_argument &err) {
      throw std::invalid_argument("Failed to parse '" + s +
                                  "' as scientific notation: " + err.what());
    } catch (const std::range_error &err) {
      throw std::range_error("Failed to parse '" + s +
                             "' as scientific notation: " + err.what());
    }
  }
};

template <class T> struct parse_number<T, chars_format::fixed> {
  auto operator()(std::string const &s) -> T {
    if (auto r = consume_hex_prefix(s); r.is_hexadecimal) {
      throw std::invalid_argument{
          "chars_format::fixed does not parse hexfloat"};
    }
    if (auto r = consume_binary_prefix(s); r.is_binary) {
      throw std::invalid_argument{
          "chars_format::fixed does not parse binfloat"};
    }
    if (s.find_first_of("eE") != std::string::npos) {
      throw std::invalid_argument{
          "chars_format::fixed does not parse exponent part"};
    }

    try {
      return do_strtod<T>(s);
    } catch (const std::invalid_argument &err) {
      throw std::invalid_argument("Failed to parse '" + s +
                                  "' as fixed notation: " + err.what());
    } catch (const std::range_error &err) {
      throw std::range_error("Failed to parse '" + s +
                             "' as fixed notation: " + err.what());
    }
  }
};

template <typename StrIt>
std::string join(StrIt first, StrIt last, const std::string &separator) {
    __builtin_trap() /* STUB: not implemented */;
}

template <typename T> struct can_invoke_to_string {
  template <typename U>
  static auto test(int)
      -> decltype(std::to_string(std::declval<U>()), std::true_type{});

  template <typename U> static auto test(...) -> std::false_type;

  static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T> struct IsChoiceTypeSupported {
  using CleanType = typename std::decay<T>::type;
  static const bool value = std::is_integral<CleanType>::value ||
                            std::is_same<CleanType, std::string>::value ||
                            std::is_same<CleanType, std::string_view>::value ||
                            std::is_same<CleanType, const char *>::value;
};

template <typename StringType>
std::size_t get_levenshtein_distance(const StringType &s1,
                                     const StringType &s2) {
    __builtin_trap() /* STUB: not implemented */;
}

template <typename ValueType>
std::string get_most_similar_string(const std::map<std::string, ValueType> &map,
                                    const std::string &input) {
    __builtin_trap() /* STUB: not implemented */;
}

} // namespace details

enum class nargs_pattern { optional, any, at_least_one };

enum class default_arguments : unsigned int {
  none = 0,
  help = 1,
  version = 2,
  all = help | version,
};

inline default_arguments operator&(const default_arguments &a,
                                   const default_arguments &b) {
    __builtin_trap() /* STUB: not implemented */;
}

class ArgumentParser;

class Argument {
  friend class ArgumentParser;
  friend auto operator<<(std::ostream &stream, const ArgumentParser &parser)
      -> std::ostream &;

  template <std::size_t N, std::size_t... I>
  explicit Argument(std::string_view prefix_chars,
                    std::array<std::string_view, N> &&a,
                    std::index_sequence<I...> /*unused*/)
      : m_accepts_optional_like_value(false),
        m_is_optional((is_optional(a[I], prefix_chars) || ...)),
        m_is_required(false), m_is_repeatable(false), m_is_used(false),
        m_is_hidden(false), m_prefix_chars(prefix_chars) {
    __builtin_trap() /* STUB: not implemented */;
}

public:
  template <std::size_t N>
  explicit Argument(std::string_view prefix_chars,
                    std::array<std::string_view, N> &&a)
      : Argument(prefix_chars, std::move(a), std::make_index_sequence<N>{}) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &help(std::string help_text) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &metavar(std::string metavar) {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T> Argument &default_value(T &&value) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &default_value(const char *value) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &required() {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &implicit_value(std::any value) {
    __builtin_trap() /* STUB: not implemented */;
}

  // This is shorthand for:
  //   program.add_argument("foo")
  //     .default_value(false)
  //     .implicit_value(true)
  Argument &flag() {
    __builtin_trap() /* STUB: not implemented */;
}

  template <class F, class... Args>
  auto action(F &&callable, Args &&... bound_args)
      -> std::enable_if_t<std::is_invocable_v<F, Args..., std::string const>,
                          Argument &> {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(bool &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
  auto &store_into(T &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
  auto &store_into(T &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(std::string &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(std::filesystem::path &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(std::vector<std::string> &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(std::vector<int> &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(std::set<std::string> &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &store_into(std::set<int> &var) {
    __builtin_trap() /* STUB: not implemented */;
}

  auto &append() {
    __builtin_trap() /* STUB: not implemented */;
}

  // Cause the argument to be invisible in usage and help
  auto &hidden() {
    __builtin_trap() /* STUB: not implemented */;
}

  template <char Shape, typename T>
  auto scan() -> std::enable_if_t<std::is_arithmetic_v<T>, Argument &> {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &nargs(std::size_t num_args) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &nargs(std::size_t num_args_min, std::size_t num_args_max) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &nargs(nargs_pattern pattern) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &remaining() {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T> void add_choice(T &&choice) {
    __builtin_trap() /* STUB: not implemented */;
}

  Argument &choices() {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T, typename... U>
  Argument &choices(T &&first, U &&... rest) {
    __builtin_trap() /* STUB: not implemented */;
}

  void find_default_value_in_choices_or_throw() const {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename Iterator>
  bool is_value_in_choices(Iterator option_it) const {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename Iterator>
  void throw_invalid_arguments_error(Iterator option_it) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /* The dry_run parameter can be set to true to avoid running the actions,
   * and setting m_is_used. This may be used by a pre-processing step to do
   * a first iteration over arguments.
   */
  template <typename Iterator>
  Iterator consume(Iterator start, Iterator end,
                   std::string_view used_name = {}, bool dry_run = false) {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * @throws std::runtime_error if argument values are not valid
   */
  void validate() const {
    __builtin_trap() /* STUB: not implemented */;
}

  std::string get_names_csv(char separator = ',') const {
    __builtin_trap() /* STUB: not implemented */;
}

  std::string get_usage_full() const {
    __builtin_trap() /* STUB: not implemented */;
}

  std::string get_inline_usage() const {
    __builtin_trap() /* STUB: not implemented */;
}

  std::size_t get_arguments_length() const {
    __builtin_trap() /* STUB: not implemented */;
}

  friend std::ostream &operator<<(std::ostream &stream,
                                  const Argument &argument) {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T> bool operator!=(const T &rhs) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * Compare to an argument value of known type
   * @throws std::logic_error in case of incompatible types
   */
  template <typename T> bool operator==(const T &rhs) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * positional:
   *    _empty_
   *    '-'
   *    '-' decimal-literal
   *    !'-' anything
   */
  static bool is_positional(std::string_view name,
                            std::string_view prefix_chars) {
    __builtin_trap() /* STUB: not implemented */;
}

private:
  class NArgsRange {
    std::size_t m_min;
    std::size_t m_max;

  public:
    NArgsRange(std::size_t minimum, std::size_t maximum)
        : m_min(minimum), m_max(maximum) {
    __builtin_trap() /* STUB: not implemented */;
}

    bool contains(std::size_t value) const {
    __builtin_trap() /* STUB: not implemented */;
}

    bool is_exact() const {
    __builtin_trap() /* STUB: not implemented */;
}

    bool is_right_bounded() const {
    __builtin_trap() /* STUB: not implemented */;
}

    std::size_t get_min() const {
    __builtin_trap() /* STUB: not implemented */;
}

    std::size_t get_max() const {
    __builtin_trap() /* STUB: not implemented */;
}

    // Print help message
    friend auto operator<<(std::ostream &stream, const NArgsRange &range)
        -> std::ostream & {
    __builtin_trap() /* STUB: not implemented */;
}

    bool operator==(const NArgsRange &rhs) const {
    __builtin_trap() /* STUB: not implemented */;
}

    bool operator!=(const NArgsRange &rhs) const {
    __builtin_trap() /* STUB: not implemented */;
}
  };

  void throw_nargs_range_validation_error() const {
    __builtin_trap() /* STUB: not implemented */;
}

  void throw_required_arg_not_used_error() const {
    __builtin_trap() /* STUB: not implemented */;
}

  void throw_required_arg_no_value_provided_error() const {
    __builtin_trap() /* STUB: not implemented */;
}

  static constexpr int eof = std::char_traits<char>::eof();

  static auto lookahead(std::string_view s) -> int {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * decimal-literal:
   *    '0'
   *    nonzero-digit digit-sequence_opt
   *    integer-part fractional-part
   *    fractional-part
   *    integer-part '.' exponent-part_opt
   *    integer-part exponent-part
   *
   * integer-part:
   *    digit-sequence
   *
   * fractional-part:
   *    '.' post-decimal-point
   *
   * post-decimal-point:
   *    digit-sequence exponent-part_opt
   *
   * exponent-part:
   *    'e' post-e
   *    'E' post-e
   *
   * post-e:
   *    sign_opt digit-sequence
   *
   * sign: one of
   *    '+' '-'
   */
  static bool is_decimal_literal(std::string_view s) {
    __builtin_trap() /* STUB: not implemented */;
}

  static bool is_optional(std::string_view name,
                          std::string_view prefix_chars) {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * Get argument value given a type
   * @throws std::logic_error in case of incompatible types
   */
  template <typename T> T get() const {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * Get argument value given a type.
   * @pre The object has no default value.
   * @returns The stored value if any, std::nullopt otherwise.
   */
  template <typename T> auto present() const -> std::optional<T> {
    __builtin_trap() /* STUB: not implemented */;
}

  template <typename T>
  static auto any_cast_container(const std::vector<std::any> &operand) -> T {
    __builtin_trap() /* STUB: not implemented */;
}

  void set_usage_newline_counter(int i) {
    __builtin_trap() /* STUB: not implemented */;
}

  void set_group_idx(std::size_t i) {
    __builtin_trap() /* STUB: not implemented */;
}

  std::vector<std::string> m_names;
  std::string_view m_used_name;
  std::string m_help;
  std::string m_metavar;
  std::any m_default_value;
  std::string m_default_value_repr;
  std::optional<std::string>
      m_default_value_str; // used for checking default_value against choices
  std::any m_implicit_value;
  std::optional<std::vector<std::string>> m_choices{std::nullopt};
  using valued_action = std::function<std::any(const std::string &)>;
  using void_action = std::function<void(const std::string &)>;
  std::vector<std::variant<valued_action, void_action>> m_actions;
  std::variant<valued_action, void_action> m_default_action{
    std::in_place_type<valued_action>,
    [](const std::string &value) { return value; }};
  std::vector<std::any> m_values;
  NArgsRange m_num_args_range{1, 1};
  // Bit field of bool values. Set default value in ctor.
  bool m_accepts_optional_like_value : 1;
  bool m_is_optional : 1;
  bool m_is_required : 1;
  bool m_is_repeatable : 1;
  bool m_is_used : 1;
  bool m_is_hidden : 1;            // if set, does not appear in usage or help
  std::string_view m_prefix_chars; // ArgumentParser has the prefix_chars
  int m_usage_newline_counter = 0;
  std::size_t m_group_idx = 0;
};

class ArgumentParser {
public:
  explicit ArgumentParser(std::string program_name = {},
                          std::string version = "1.0",
                          default_arguments add_args = default_arguments::all,
                          bool exit_on_default_arguments = true,
                          std::ostream &os = std::cout)
      : m_program_name(std::move(program_name)), m_version(std::move(version)),
        m_exit_on_default_arguments(exit_on_default_arguments),
        m_parser_path(m_program_name) {
    __builtin_trap() /* STUB: not implemented */;
}

  ~ArgumentParser() = default;

  // ArgumentParser is meant to be used in a single function.
  // Setup everything and parse arguments in one place.
  //
  // ArgumentParser internally uses std::string_views,
  // references, iterators, etc.
  // Many of these elements become invalidated after a copy or move.
  ArgumentParser(const ArgumentParser &other) = delete;
  ArgumentParser &operator=(const ArgumentParser &other) = delete;
  ArgumentParser(ArgumentParser &&) noexcept = delete;
  ArgumentParser &operator=(ArgumentParser &&) = delete;

  explicit operator bool() const {
    __builtin_trap() /* STUB: not implemented */;
}

  // Parameter packing
  // Call add_argument with variadic number of string arguments
  template <typename... Targs> Argument &add_argument(Targs... f_args) {
    __builtin_trap() /* STUB: not implemented */;
}

  class MutuallyExclusiveGroup {
    friend class ArgumentParser;

  public:
    MutuallyExclusiveGroup() = delete;

    explicit MutuallyExclusiveGroup(ArgumentParser &parent,
                                    bool required = false)
        : m_parent(parent), m_required(required), m_elements({}) {
    __builtin_trap() /* STUB: not implemented */;
}

    MutuallyExclusiveGroup(const MutuallyExclusiveGroup &other) = delete;
    MutuallyExclusiveGroup &
    operator=(const MutuallyExclusiveGroup &other) = delete;

    MutuallyExclusiveGroup(MutuallyExclusiveGroup &&other) noexcept
        : m_parent(other.m_parent), m_required(other.m_required),
          m_elements(std::move(other.m_elements)) {
      other.m_elements.clear();
    }

    template <typename... Targs> Argument &add_argument(Targs... f_args) {
    __builtin_trap() /* STUB: not implemented */;
}

  private:
    ArgumentParser &m_parent;
    bool m_required{false};
    std::vector<Argument *> m_elements{};
  };

  MutuallyExclusiveGroup &add_mutually_exclusive_group(bool required = false) {
    __builtin_trap() /* STUB: not implemented */;
}

  // Parameter packed add_parents method
  // Accepts a variadic number of ArgumentParser objects
  template <typename... Targs>
  ArgumentParser &add_parents(const Targs &... f_args) {
    __builtin_trap() /* STUB: not implemented */;
}

  // Ask for the next optional arguments to be displayed on a separate
  // line in usage() output. Only effective if set_usage_max_line_width() is
  // also used.
  ArgumentParser &add_usage_newline() {
    __builtin_trap() /* STUB: not implemented */;
}

  // Ask for the next optional arguments to be displayed in a separate section
  // in usage() and help (<< *this) output.
  // For usage(), this is only effective if set_usage_max_line_width() is
  // also used.
  ArgumentParser &add_group(std::string group_name) {
    __builtin_trap() /* STUB: not implemented */;
}

  ArgumentParser &add_description(std::string description) {
    __builtin_trap() /* STUB: not implemented */;
}

  ArgumentParser &add_epilog(std::string epilog) {
    __builtin_trap() /* STUB: not implemented */;
}

  // Add a un-documented/hidden alias for an argument.
  // Ideally we'd want this to be a method of Argument, but Argument
  // does not own its owing ArgumentParser.
  ArgumentParser &add_hidden_alias_for(Argument &arg, std::string_view alias) {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Getter for arguments and subparsers.
   * @throws std::logic_error in case of an invalid argument or subparser name
   */
  template <typename T = Argument> T &at(std::string_view name) {
    __builtin_trap() /* STUB: not implemented */;
}

  ArgumentParser &set_prefix_chars(std::string prefix_chars) {
    __builtin_trap() /* STUB: not implemented */;
}

  ArgumentParser &set_assign_chars(std::string assign_chars) {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Call parse_args_internal - which does all the work
   * Then, validate the parsed arguments
   * This variant is used mainly for testing
   * @throws std::runtime_error in case of any invalid argument
   */
  void parse_args(const std::vector<std::string> &arguments) {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Call parse_known_args_internal - which does all the work
   * Then, validate the parsed arguments
   * This variant is used mainly for testing
   * @throws std::runtime_error in case of any invalid argument
   */
  std::vector<std::string>
  parse_known_args(const std::vector<std::string> &arguments) {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Main entry point for parsing command-line arguments using this
   * ArgumentParser
   * @throws std::runtime_error in case of any invalid argument
   */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
  void parse_args(int argc, const char *const argv[]) {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Main entry point for parsing command-line arguments using this
   * ArgumentParser
   * @throws std::runtime_error in case of any invalid argument
   */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
  auto parse_known_args(int argc, const char *const argv[]) {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Getter for options with default values.
   * @throws std::logic_error if parse_args() has not been previously called
   * @throws std::logic_error if there is no such option
   * @throws std::logic_error if the option has no value
   * @throws std::bad_any_cast if the option is not of type T
   */
  template <typename T = std::string> T get(std::string_view arg_name) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Getter for options without default values.
   * @pre The option has no default value.
   * @throws std::logic_error if there is no such option
   * @throws std::bad_any_cast if the option is not of type T
   */
  template <typename T = std::string>
  auto present(std::string_view arg_name) const -> std::optional<T> {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Getter that returns true for user-supplied options. Returns false if not
   * user-supplied, even with a default value.
   */
  auto is_used(std::string_view arg_name) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Getter that returns true if a subcommand is used.
   */
  auto is_subcommand_used(std::string_view subcommand_name) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Getter that returns true if a subcommand is used.
   */
  auto is_subcommand_used(const ArgumentParser &subparser) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /* Indexing operator. Return a reference to an Argument object
   * Used in conjunction with Argument.operator== e.g., parser["foo"] == true
   * @throws std::logic_error in case of an invalid argument name
   */
  Argument &operator[](std::string_view arg_name) const {
    __builtin_trap() /* STUB: not implemented */;
}

  // Print help message
  friend auto operator<<(std::ostream &stream, const ArgumentParser &parser)
      -> std::ostream & {
    __builtin_trap() /* STUB: not implemented */;
}

  // Format help message
  auto help() const -> std::stringstream {
    __builtin_trap() /* STUB: not implemented */;
}

  // Sets the maximum width for a line of the Usage message
  ArgumentParser &set_usage_max_line_width(size_t w) {
    __builtin_trap() /* STUB: not implemented */;
}

  // Asks to display arguments of mutually exclusive group on separate lines in
  // the Usage message
  ArgumentParser &set_usage_break_on_mutex() {
    __builtin_trap() /* STUB: not implemented */;
}

  // Format usage part of help only
  auto usage() const -> std::string {
    __builtin_trap() /* STUB: not implemented */;
}

  // Printing the one and only help message
  // I've stuck with a simple message format, nothing fancy.
  [[deprecated("Use cout << program; instead.  See also help().")]] std::string
  print_help() const {
    __builtin_trap() /* STUB: not implemented */;
}

  void add_subparser(ArgumentParser &parser) {
    __builtin_trap() /* STUB: not implemented */;
}

  void set_suppress(bool suppress) {
    __builtin_trap() /* STUB: not implemented */;
}

protected:
  const MutuallyExclusiveGroup *get_belonging_mutex(const Argument *arg) const {
    __builtin_trap() /* STUB: not implemented */;
}

  bool is_valid_prefix_char(char c) const {
    __builtin_trap() /* STUB: not implemented */;
}

  char get_any_valid_prefix_char() const {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * Pre-process this argument list. Anything starting with "--", that
   * contains an =, where the prefix before the = has an entry in the
   * options table, should be split.
   */
  std::vector<std::string>
  preprocess_arguments(const std::vector<std::string> &raw_arguments) const {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * @throws std::runtime_error in case of any invalid argument
   */
  void parse_args_internal(const std::vector<std::string> &raw_arguments) {
    __builtin_trap() /* STUB: not implemented */;
}

  /*
   * Like parse_args_internal but collects unused args into a vector<string>
   */
  std::vector<std::string>
  parse_known_args_internal(const std::vector<std::string> &raw_arguments) {
    __builtin_trap() /* STUB: not implemented */;
}

  // Used by print_help.
  std::size_t get_length_of_longest_argument() const {
    __builtin_trap() /* STUB: not implemented */;
}

  using argument_it = std::list<Argument>::iterator;
  using mutex_group_it = std::vector<MutuallyExclusiveGroup>::iterator;
  using argument_parser_it =
      std::list<std::reference_wrapper<ArgumentParser>>::iterator;

  void index_argument(argument_it it) {
    __builtin_trap() /* STUB: not implemented */;
}

  std::string m_program_name;
  std::string m_version;
  std::string m_description;
  std::string m_epilog;
  bool m_exit_on_default_arguments = true;
  std::string m_prefix_chars{"-"};
  std::string m_assign_chars{"="};
  bool m_is_parsed = false;
  std::list<Argument> m_positional_arguments;
  std::list<Argument> m_optional_arguments;
  std::map<std::string, argument_it> m_argument_map;
  std::string m_parser_path;
  std::list<std::reference_wrapper<ArgumentParser>> m_subparsers;
  std::map<std::string, argument_parser_it> m_subparser_map;
  std::map<std::string, bool> m_subparser_used;
  std::vector<MutuallyExclusiveGroup> m_mutually_exclusive_groups;
  bool m_suppress = false;
  std::size_t m_usage_max_line_width = (std::numeric_limits<std::size_t>::max)();
  bool m_usage_break_on_mutex = false;
  int m_usage_newline_counter = 0;
  std::vector<std::string> m_group_names;
};

} // namespace argparse
