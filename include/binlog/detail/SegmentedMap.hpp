#ifndef BINLOG_DETAIL_SEGMENTEDMAP_H_
#define BINLOG_DETAIL_SEGMENTEDMAP_H_

#include <cstdint>
#include <utility> // forward
#include <vector>

namespace binlog {
namespace detail {

/**
 * A map<size_t, V> like container,
 * optimized for mostly numerically contiguous keys.
 *
 * Values of contiguous keys are stored
 * in a vector - contiguously.
 */
template <typename V>
class SegmentedMap
{
public:
  using key_type = std::uint64_t;
  using mapped_type = V;

private:
  using Segment = std::vector<mapped_type>;

  // Invariant: _offsets[i] == key_of(_segments[i][0])
  // Invariant: key_of(_segments[i][j]) + 1 == key_of(_segments[i][j + 1])
  //
  // _offsets is never empty, to make lookup cheaper

  std::vector<key_type> _offsets{0};
  std::vector<Segment> _segments{{}};

public:
  template <class... Args>
  void emplace(const key_type key, Args&&... args)
  {
    const key_type si = segmentIndex(key);
    const key_type offset = _offsets[si];
    const key_type vi = key - offset;
    Segment& segment = _segments[si];

    if (segment.size() == vi)
    {
      segment.emplace_back(std::forward<Args>(args)...);
    }
    else if (segment.size() > vi)
    {
      segment[vi] = mapped_type(std::forward<Args>(args)...);
    }
    else
    {
      const auto ito = std::vector<key_type>::iterator::difference_type(si + 1);
      _offsets.insert(_offsets.begin() + ito, key);
      _segments.emplace(_segments.begin() + ito, Segment{mapped_type(std::forward<Args>(args)...)});
    }
  }

  bool empty() const
  {
    return size() == 0;
  }

  std::size_t size() const
  {
    std::size_t s = 0;
    for (const Segment& segment : _segments)
    {
      s += segment.size();
    }
    return s;
  }

  mapped_type* end() const { return nullptr; }

  const mapped_type* find(const key_type& key) const
  {
    const key_type si = segmentIndex(key);
    const key_type offset = _offsets[si];
    const key_type vi = key - offset;
    const Segment& segment = _segments[si];

    if (vi < segment.size())
    {
      return & segment[vi];
    }

    return end();
  }

private:
  key_type segmentIndex(const key_type key) const
  {
    key_type si = 1;
    for (; si < _offsets.size() && _offsets[si] <= key; ++si) { /* nop */; }
    return si - 1;
  }
};

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_SEGMENTEDMAP_H_
