#include "tests.h"
#include "utils/test_runner.h"

#include <functional>
#include <string>
#include <unordered_map>

using namespace std;

const static unordered_map<string, function<void()>> tests = {
    {"1", TestIntegrationTest1},
    {"2", TestIntegrationTest2},
    {"3", TestIntegrationTest3},
    {"4", TestIntegrationTest4},
};

int main(int argc, char *argv[]) {
  TestRunner tr;

  if (argc == 1) {
    for (auto &[_, test] : tests) {
        RUN_TEST(tr, test);
    }
  } else {
    if (auto it = tests.find(argv[1]); it != tests.end()) {
      RUN_TEST(tr, it->second);
    } else {
      cerr << "Test not found";
      return 1;
    }
  }
  return 0;
}