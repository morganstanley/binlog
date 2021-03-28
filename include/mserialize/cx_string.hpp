#ifndef MSERIALIZE_CX_STRING_HPP
#define MSERIALIZE_CX_STRING_HPP

#include <cstddef> // size_t

#include <mserialize/string_view.hpp>

namespace mserialize {

/**
 * A fixed-size, null terminated string for constexpr manipulation.
 *
 * The string can contain embedded null characters.
 *
 * @invariant (*this)[N] == 0
 *
 * Considered alternatives:
 * std::array lacks necessary constexpr accessors.
 * C array cannot be returned by value.
 */
template <std::size_t N>
class cx_string
{
public:
  using data_ref = const char(&)[N+1];

  /**
   * Construct a cx_string from `str`.
   *
   * @pre str[0,N) must be valid
   * @post strncmp(data(), str, N) == 0
   */
  explicit constexpr cx_string(const char* str)
    :_data{0}
  {
    #if defined(__GNUC__) && !defined(__clang__)
      // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96742
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wtype-limits"
    #endif
    for (std::size_t i = 0; i < N; ++i)
    {
      _data[i] = str[i];
    }
    #if defined(__GNUC__) && !defined(__clang__)
      #pragma GCC diagnostic pop
    #endif
  }

  /**
   * Access the i'th character.
   *
   * @pre i <= size()
   */
  constexpr char operator[](std::size_t i) const { return _data[i]; }

  /** @return a const reference to the underlying character array */
  constexpr data_ref data() const { return _data; }

  /** @return size of the string, does not include the closing zero */
  constexpr std::size_t size() const { return N; }

  /** @return iterator pointing to the first character, if any */
  constexpr const char* begin() const { return _data; }

  /** @return iterator pointing to the closing zero */
  constexpr const char* end() const { return _data+N; }

  /** @return a string_view of the underlying character array */
  operator string_view() const { return {_data, N}; } // NOLINT(google-explicit-constructor)

private:
  char _data[N+1]; // +1: closing zero
};

namespace detail {

template <std::size_t... N>
constexpr std::size_t sum()
{
  std::size_t result = 0;
  const std::size_t ns[] = {0, N...};
  for (auto n : ns) { result += n; }
  return result;
}

} // namespace detail

/** Concatenate any number of cx_string objects. */
template <std::size_t... N>
constexpr auto cx_strcat(const cx_string<N>&&... strings)
{
  char buffer[detail::sum<N...>()+1] = {0};
  const char* data[] = {"", strings.data()...};
  const std::size_t size[] = {0, strings.size()...};

  char* out = buffer;
  for (std::size_t i = 1; i <= sizeof...(N); ++i)
  {
    for (std::size_t s = 0; s < size[i]; ++s) { *out++ = data[i][s]; } // NOLINT
  }

  return cx_string<sizeof(buffer)-1>(buffer);
}

/** Create a cx_string from a string literal */
template <std::size_t N>
constexpr cx_string<N-1> make_cx_string(const char (&str)[N])
{
  return cx_string<N-1>(str);
}

} // namespace mserialize

#endif // MSERIALIZE_CX_STRING_HPP
