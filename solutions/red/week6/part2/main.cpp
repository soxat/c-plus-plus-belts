#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "parse.h"
#include "search_server.h"
#include "test_runner.h"

using namespace std;

void TestFunctionality(const vector<string> &docs, const vector<string> &queries, const vector<string> &expected) {
  istringstream docs_input(Join('\n', docs));
  istringstream queries_input(Join('\n', queries));

  SearchServer srv;
  srv.UpdateDocumentBaseSync(docs_input);
  ostringstream queries_output;
  srv.AddQueriesStreamSync(queries_input, queries_output);

  const string result = queries_output.str();
  const auto lines = SplitBy(Strip(result), '\n');
  ASSERT_EQUAL(lines.size(), expected.size());
  for (size_t i = 0; i < lines.size(); ++i) {
    ASSERT_EQUAL(lines[i], expected[i]);
  }
}

void TestSerpFormat() {
  const vector<string> docs = {"london is the capital of great britain", "i am travelling down the river"};
  const vector<string> queries = {"london", "the"};
  const vector<string> expected = {"london: {docid: 0, hitcount: 1}",
                                   Join(' ', vector{"the:", "{docid: 0, hitcount: 1}", "{docid: 1, hitcount: 1}"})};

  TestFunctionality(docs, queries, expected);
}

void TestTop5() {
  const vector<string> docs = {"milk a", "milk b", "milk c",  "milk d",  "milk e",
                               "milk f", "milk g", "water a", "water b", "fire and earth"};

  const vector<string> queries = {"milk", "water", "rock"};
  const vector<string> expected = {
      Join(' ', vector{"milk:", "{docid: 0, hitcount: 1}", "{docid: 1, hitcount: 1}", "{docid: 2, hitcount: 1}",
                       "{docid: 3, hitcount: 1}", "{docid: 4, hitcount: 1}"}),
      Join(' ',
           vector{
               "water:",
               "{docid: 7, hitcount: 1}",
               "{docid: 8, hitcount: 1}",
           }),
      "rock:",
  };
  TestFunctionality(docs, queries, expected);
}

void TestHitcount() {
  const vector<string> docs = {
      "the river goes through the entire city there is a house near it",
      "the wall",
      "walle",
      "is is is is",
  };
  const vector<string> queries = {"the", "wall", "all", "is", "the is"};
  const vector<string> expected = {
      Join(' ',
           vector{
               "the:",
               "{docid: 0, hitcount: 2}",
               "{docid: 1, hitcount: 1}",
           }),
      "wall: {docid: 1, hitcount: 1}",
      "all:",
      Join(' ',
           vector{
               "is:",
               "{docid: 3, hitcount: 4}",
               "{docid: 0, hitcount: 1}",
           }),
      Join(' ',
           vector{
               "the is:",
               "{docid: 3, hitcount: 4}",
               "{docid: 0, hitcount: 3}",
               "{docid: 1, hitcount: 1}",
           }),
  };
  TestFunctionality(docs, queries, expected);
}

void TestRanking() {
  const vector<string> docs = {
      "london is the capital of great britain",
      "paris is the capital of france",
      "berlin is the capital of germany",
      "rome is the capital of italy",
      "madrid is the capital of spain",
      "lisboa is the capital of portugal",
      "bern is the capital of switzerland",
      "moscow is the capital of russia",
      "kiev is the capital of ukraine",
      "minsk is the capital of belarus",
      "astana is the capital of kazakhstan",
      "beijing is the capital of china",
      "tokyo is the capital of japan",
      "bangkok is the capital of thailand",
      "welcome to moscow the capital of russia the third rome",
      "amsterdam is the capital of netherlands",
      "helsinki is the capital of finland",
      "oslo is the capital of norway",
      "stockgolm is the capital of sweden",
      "riga is the capital of latvia",
      "tallin is the capital of estonia",
      "warsaw is the capital of poland",
  };

  const vector<string> queries = {"moscow is the capital of russia"};
  const vector<string> expected = {Join(' ', vector{
                                                 "moscow is the capital of russia:",
                                                 "{docid: 7, hitcount: 6}",
                                                 "{docid: 14, hitcount: 6}",
                                                 "{docid: 0, hitcount: 4}",
                                                 "{docid: 1, hitcount: 4}",
                                                 "{docid: 2, hitcount: 4}",
                                             })};
  TestFunctionality(docs, queries, expected);
}

void TestBasicSearch() {
  const vector<string> docs = {"we are ready to go",
                               "come on everybody shake you hands",
                               "i love this game",
                               "just like exception safety is not about writing try catch everywhere in "
                               "your code move "
                               "semantics are not about typing double ampersand everywhere in your code",
                               "daddy daddy daddy dad dad dad",
                               "tell me the meaning of being lonely",
                               "just keep track of it",
                               "how hard could it be",
                               "it is going to be legen wait for it dary legendary",
                               "we dont need no education"};

  const vector<string> queries = {"we need some help", "it", "i love this game", "tell me why", "dislike", "about"};

  const vector<string> expected = {
      Join(' ', vector{"we need some help:", "{docid: 9, hitcount: 2}", "{docid: 0, hitcount: 1}"}),
      Join(' ',
           vector{
               "it:",
               "{docid: 8, hitcount: 2}",
               "{docid: 6, hitcount: 1}",
               "{docid: 7, hitcount: 1}",
           }),
      "i love this game: {docid: 2, hitcount: 4}",
      "tell me why: {docid: 5, hitcount: 2}",
      "dislike:",
      "about: {docid: 3, hitcount: 2}",
  };
  TestFunctionality(docs, queries, expected);
}

struct LargeFileOptions {
  const string &filename;
  default_random_engine &rd;
  const vector<string> &possible_words;
  size_t lines_count;
  size_t max_words_per_line;
  size_t max_word_length;
  int possible_words_presence_percentage;
  function<string(size_t)> random_word;

  void Generate() {
    std::uniform_int_distribution<size_t> len_gen(1, max_word_length);

    cerr << "Generating " << filename << "..." << endl;
    std::uniform_int_distribution<size_t> line_len_gen(1, max_words_per_line);
    std::uniform_int_distribution<int> word_choice(1, 100);
    ofstream file_out(filename);

    for (size_t line = 0; line < lines_count; ++line) {
      cerr << "\r-> Generating " << line + 1 << " of " << lines_count;
      ostringstream line_out;
      auto line_len = line_len_gen(rd);
      for (size_t i = 0; i < line_len; ++i) {
        if (word_choice(rd) <= possible_words_presence_percentage) {
          std::uniform_int_distribution<size_t> word_index(0, possible_words.size() - 1);
          line_out << possible_words[word_index(rd)];
        } else {
          line_out << random_word(len_gen(rd));
        }
        line_out << ' ';
      }
      file_out << line_out.str() << '\n';
    }
    cerr << endl;
  }
};

void GenerateLargeTextFiles() {
  cerr << "Starting generators..." << endl;
  std::default_random_engine rd(34);
  std::uniform_int_distribution<char> char_gen('a', 'z');

  size_t max_word_len = 100;
  auto random_word = [&](size_t len) {
    string result(len, ' ');
    std::generate(begin(result), end(result), [&] { return char_gen(rd); });
    return result;
  };

  std::uniform_int_distribution<size_t> len_gen(1, max_word_len);

  cerr << "Generating possible words pool..." << endl;
  vector<string> possible_words(3'0);
  for (size_t i = 0; i < possible_words.size(); ++i) {
    cerr << "\r-> Generating " << i + 1 << " of " << possible_words.size();
    possible_words[i] = random_word(len_gen(rd));
  }
  cerr << endl;

  LargeFileOptions database{
      "database.txt",
      rd,
      possible_words,
      11000,  // 50000
      50,
      max_word_len,
      80,
      random_word,
  };

  database.Generate();

  LargeFileOptions queries{
      "queries.txt",
      rd,
      possible_words,
      12000,  // 500000
      50,
      max_word_len,
      80,
      random_word,
  };

  queries.Generate();

  cerr << "Generation complete" << endl;
}

void TestSpeedUp() {
  ifstream docs_input("database.txt");
  ifstream queries_input("queries.txt");

  SearchServer srv;
  {
    LOG_DURATION("UpdateDocumentBase");
    srv.UpdateDocumentBaseSync(docs_input);
  }
  ostringstream queries_output;
  {
    LOG_DURATION("AddQueriesStream")
    srv.AddQueriesStreamSync(queries_input, queries_output);
  }
}

void TestSearchServer(vector<pair<istream *, ostream *>> streams) {
  TotalDuration updateDocumentBase("UpdateDocumentBase");
  TotalDuration addQueriesStream("addQueriesStream");
  LOG_DURATION("SRV LIVE");
  std::mt19937_64 eng{std::random_device{}()};  // or seed however you want
  std::uniform_int_distribution<> dist{0, 100};
  SearchServer srv(*streams.front().first);
  for (auto &[input, output] : IteratorRange(begin(streams) + 1, end(streams))) {
    this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
    if (!output) {
      ADD_DURATION(updateDocumentBase)
      srv.UpdateDocumentBase(*input);
    } else {
      ADD_DURATION(addQueriesStream);
      srv.AddQueriesStream(*input, *output);
    }
  }
}

void TestMultithread() {
  {
    ifstream docs1("database1.txt");
    ifstream docs2("database2.txt");
    ifstream docs3("database3.txt");
    ifstream docs4("database4.txt");
    ifstream docs5("database5.txt");
    ifstream docs6("database6.txt");
    ifstream qf1("queries1.txt");
    ifstream qf2("queries2.txt");
    ifstream qf3("queries3.txt");
    ifstream qf4("queries4.txt");
    ifstream qf5("queries5.txt");
    ifstream qf6("queries6.txt");

    ofstream of1("results1.txt");
    ofstream of2("results2.txt");
    ofstream of3("results3.txt");
    ofstream of4("results4.txt");
    ofstream of5("results5.txt");
    ofstream of6("results6.txt");
    vector<pair<istream *, ostream *>> streams{
        make_pair(&docs1, nullptr), make_pair(&qf1, &of1),      make_pair(&docs2, nullptr), make_pair(&qf2, &of2),
        make_pair(&qf3, &of3),      make_pair(&docs3, nullptr), make_pair(&qf4, &of4),      make_pair(&docs4, nullptr),
        make_pair(&qf5, &of5),      make_pair(&docs5, nullptr), make_pair(&qf6, &of6),      make_pair(&docs6, nullptr),
    };

    TestSearchServer(streams);
  }
}

int main() {
  TestRunner tr;

  RUN_TEST(tr, TestSerpFormat);
  RUN_TEST(tr, TestTop5);
  RUN_TEST(tr, TestHitcount);
  RUN_TEST(tr, TestRanking);
  RUN_TEST(tr, TestBasicSearch);
  // RUN_TEST(tr, GenerateLargeTextFiles);
  // RUN_TEST(tr, TestSpeedUp);
  RUN_TEST(tr, TestMultithread);
  return 0;
}
