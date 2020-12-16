#pragma once

#include <functional>
#include <map>

template <class KEY, class VALUE> class MutableMap {
public:
  VALUE operator()(KEY key) const {
    auto findit = map.find(key);
    if (findit != map.end()) {
      return findit->second;
    }
    return map[key] = fnGenerator(key);
  }

  std::function<VALUE(KEY)> fnGenerator;

private:
  mutable std::map<KEY, VALUE> map;
};