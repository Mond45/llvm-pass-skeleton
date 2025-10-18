#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

std::unordered_map<std::string, unsigned int> &call_cnts() {
  static std::unordered_map<std::string, unsigned int> *map =
      new std::unordered_map<std::string, unsigned int>();
  return *map;
}

static std::mutex map_mutex;

extern "C" void __instrumentor_summary() {
  std::cerr << "== function calls statistic ==\n";
  for (auto &[fn, cnt] : call_cnts()) {
    std::cerr << fn << ": " << cnt << '\n';
  }
}

extern "C" void __instrumentor_incr_cnt(const char *fn_name) {
  std::scoped_lock<std::mutex> guard(map_mutex);
  call_cnts()[fn_name]++;
}
