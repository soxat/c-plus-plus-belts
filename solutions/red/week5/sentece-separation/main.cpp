#include <vector>

#include "test_runner.h"

using namespace std;

// Объявляем Sentence<Token> для произвольного типа Token
// синонимом vector<Token>.
// Благодаря этому в качестве возвращаемого значения
// функции можно указать не малопонятный вектор векторов,
// а вектор предложений — vector<Sentence<Token>>.
template <typename Token>
using Sentence = vector<Token>;

// Класс Token имеет метод bool IsEndSentencePunctuation() const
template <typename Token>
vector<Sentence<Token>> SplitIntoSentences(vector<Token> tokens) {
  Sentence<Token> sentence;
  vector<Sentence<Token>> result;

  bool nextChangeCollect = false;

  for (auto &token : tokens) {
    if (token.IsEndSentencePunctuation()) {
      nextChangeCollect = true;
    } else {
      if (nextChangeCollect && !sentence.empty()) {
        nextChangeCollect = false;
        result.push_back(move(sentence));
      }
    }

    sentence.push_back(move(token));
  }

  if (!sentence.empty()) {
    result.push_back(move(sentence));
  }

  return result;
}

struct TestToken {
  string data;
  bool is_end_sentence_punctuation = false;

  bool IsEndSentencePunctuation() const { return is_end_sentence_punctuation; }

  bool operator==(const TestToken &other) const {
    return data == other.data && is_end_sentence_punctuation == other.is_end_sentence_punctuation;
  }
};

ostream &operator<<(ostream &stream, const TestToken &token) { return stream << token.data; }

// Тест содержит копирования объектов класса TestToken.
// Для проверки отсутствия копирований в функции SplitIntoSentences
// необходимо написать отдельный тест.
void TestSplitting() {
  {
    auto actual = SplitIntoSentences(vector<TestToken>({{"Split"}, {"into"}, {"sentences"}, {"!"}}));
    auto expected = vector<Sentence<TestToken>>({{{"Split"}, {"into"}, {"sentences"}, {"!"}}});

    ASSERT_EQUAL(actual, expected

    );
  }

  {
    ASSERT_EQUAL(SplitIntoSentences(vector<TestToken>({{"Split"}, {"into"}, {"sentences"}, {"!", true}})),
                 vector<Sentence<TestToken>>({{{"Split"}, {"into"}, {"sentences"}, {"!", true}}}));
  }

  {
    auto actual = SplitIntoSentences(vector<TestToken>(
        {{"Split"}, {"into"}, {"sentences"}, {"!", true}, {"!", true}, {"Without"}, {"copies"}, {".", true}}));
    auto expected = vector<Sentence<TestToken>>({
        {{"Split"}, {"into"}, {"sentences"}, {"!", true}, {"!", true}},
        {{"Without"}, {"copies"}, {".", true}},
    });
    ASSERT_EQUAL(actual, expected);
  }
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestSplitting);
  return 0;
}