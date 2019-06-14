#include <mserialize/deserialize.hpp>
#include <mserialize/serialize.hpp>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <cmath> // isnan
#include <cstdint>
#include <exception>
#include <limits>
#include <sstream>

namespace {

using arithmetic_types = boost::mpl::list<
  bool,
  std::int8_t, std::int16_t, std::int32_t, std::int64_t,
  std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
  float, double, long double
>;

using float_types = boost::mpl::list<
  float, double, long double
>;

// In tests, do not use stringstream directly,
// to make sure tested code only accesses
// members of the specific concept.

struct OutputStream
{
  std::stringstream& stream;

  OutputStream& write(const char* buf, std::streamsize size)
  {
    stream.write(buf, size);
    return *this;
  }
};

struct InputStream
{
  std::stringstream& stream;

  InputStream& read(char* buf, std::streamsize size)
  {
    stream.read(buf, size);
    return *this;
  }
};

template <typename T>
T roundtrip(const T& in)
{
  std::stringstream stream;
  stream.exceptions(std::ios_base::failbit);

  // serialize
  OutputStream ostream{stream};
  mserialize::serialize(in, ostream);

  // deserialize
  T out;
  InputStream istream{stream};
  mserialize::deserialize(out, istream);

  return out;
}

} // namespace

BOOST_AUTO_TEST_SUITE(MserializeRoundtrip)

BOOST_AUTO_TEST_CASE_TEMPLATE(arithmetic_min_max, T, arithmetic_types)
{
  // min
  {
    const T in = std::numeric_limits<T>::max();
    const T out = roundtrip(in);
    BOOST_TEST(in == out);
  }

  // max
  {
    const T in = std::numeric_limits<T>::min();
    const T out = roundtrip(in);
    BOOST_TEST(in == out);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(float_spec, T, float_types)
{
  // lowest
  {
    const T in = std::numeric_limits<T>::lowest();
    const T out = roundtrip(in);
    BOOST_TEST(in == out);
  }

  // Negative 0
  {
    const T in{-0.0};
    const T out = roundtrip(in);
    BOOST_TEST(in == out);
  }

  // -Inf
  if (std::numeric_limits<T>::has_infinity)
  {
    const T in = T{-1.} * std::numeric_limits<T>::infinity();
    const T out = roundtrip(in);
    BOOST_TEST(in == out);
  }

  // +Inf
  if (std::numeric_limits<T>::has_infinity)
  {
    const T in = std::numeric_limits<T>::infinity();
    const T out = roundtrip(in);
    BOOST_TEST(in == out);
  }

  // Quiet NaN
  if (std::numeric_limits<T>::has_quiet_NaN)
  {
    const T in = std::numeric_limits<T>::quiet_NaN();
    const T out = roundtrip(in);
    BOOST_TEST(std::isnan(out));
  }

  // Signaling NaN
  if (std::numeric_limits<T>::has_signaling_NaN)
  {
    const T in = std::numeric_limits<T>::signaling_NaN();
    const T out = roundtrip(in);
    BOOST_TEST(std::isnan(out));
  }
}

BOOST_AUTO_TEST_CASE(errorOnEof)
{
  int out;
  std::stringstream stream;
  stream.exceptions(std::ios_base::failbit);
  BOOST_CHECK_THROW(
    mserialize::deserialize(out, stream),
    std::exception
  );
}

BOOST_AUTO_TEST_CASE(errorOnIncomplete)
{
  std::stringstream stream;
  stream.exceptions(std::ios_base::failbit);
  mserialize::serialize(std::int16_t{123}, stream);

  std::int32_t out;
  BOOST_CHECK_THROW(
    mserialize::deserialize(out, stream),
    std::exception
  );
}

BOOST_AUTO_TEST_SUITE_END()
