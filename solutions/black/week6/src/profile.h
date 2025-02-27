#pragma once

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

class LogDuration {
 public:
  explicit LogDuration(const std::string &msg = "") : message(msg + ": "), start(std::chrono::steady_clock::now()) {}

  ~LogDuration() {
    auto finish = std::chrono::steady_clock::now();
    auto dur = finish - start;
    auto s_count = std::chrono::duration_cast<std::chrono::seconds>(dur).count();
    auto ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    std::cerr << message << s_count << " (s) | " << ms_count << " (ms)" << std::endl;
  }

 private:
  std::string message;
  std::chrono::steady_clock::time_point start;
};

#define UNIQ_ID_IMPL(lineno) _a_local_var_##lineno
#define UNIQ_ID(lineno) UNIQ_ID_IMPL(lineno)

#define LOG_DURATION(message) LogDuration UNIQ_ID(__LINE__){message};

struct TotalDuration {
  std::string message;
  std::chrono::steady_clock::duration value;
  explicit TotalDuration() : value(0) {}
  explicit TotalDuration(const std::string &msg) : message(msg + ": "), value(0) {}
  ~TotalDuration() {
    if (message.empty()) return;
    std::ostringstream os;
    os << message << std::chrono::duration_cast<std::chrono::milliseconds>(value).count() << " ms" << std::endl;
    std::cerr << os.str();
  }
  void Print(std::ostream& out) const {
    out << std::chrono::duration_cast<std::chrono::milliseconds>(value).count() << " ms";
  }
};
class AddDuration {
 public:
  explicit AddDuration(std::chrono::steady_clock::duration &dest)
      : add_to(dest), start(std::chrono::steady_clock::now()) {}
  explicit AddDuration(TotalDuration &dest) : AddDuration(dest.value) {}
  ~AddDuration() { add_to += std::chrono::steady_clock::now() - start; }

 private:
  std::chrono::steady_clock::duration &add_to;
  std::chrono::steady_clock::time_point start;
};

#define ADD_DURATION(value) AddDuration UNIQ_ID(__LINE__){value};

template<class T>
struct is_duration : std::false_type {};

template<class Rep, class Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};
