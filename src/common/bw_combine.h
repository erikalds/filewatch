#ifndef BW_COMBINE_H
#define BW_COMBINE_H

#include <cassert>
#include <limits>

namespace util
{
  template<typename RetT, typename T>
  RetT bw_combine(T val)
  {
    assert(val < std::numeric_limits<RetT>::max());
    return static_cast<RetT>(val);
  }

  template<typename RetT, typename T, typename... Us>
  RetT bw_combine(T val, Us... other)
  {
    assert(val < std::numeric_limits<RetT>::max());
    return static_cast<RetT>(val) | bw_combine<RetT>(other...);
  }

}  // namespace util

#endif /* BW_COMBINE_H */
