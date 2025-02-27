#include "Solution.h"

using namespace std;

int main() {
  std::istream::sync_with_stdio(false);
  cin.tie(nullptr);
  cout.precision(DEFAULT_PRECISION);

  TestRunner tr;
  //  commit tests before sending to grader
  RUN_TEST(tr, TestParsing);
  RUN_TEST(tr, TestIntegrationPartD);
  RUN_TEST(tr, TestIntegrationPartDRaw);

  Json::Document document = Json::Load(cin);
  auto modifyRequests = ParseSpecificRequests(MODIFY_TYPES_CONVERTER, document, "base_requests");
  auto readRequests = ParseSpecificRequests(READ_TYPES_CONVERTER, document, "stat_requests");
  Database db;
  ProcessModifyRequests(db, modifyRequests);
  auto jsonDoc = ProcessReadRequests(db, readRequests);
  cout << jsonDoc;

  return 0;
}
