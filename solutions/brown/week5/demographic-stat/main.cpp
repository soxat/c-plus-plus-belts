#include <iostream>
#include <vector>

#include "profile.h"
#include "test_runner.h"

using namespace std;

enum class Gender { FEMALE, MALE };

struct Person {
  int age;           // возраст
  Gender gender;     // пол
  bool is_employed;  // имеет ли работу
};

// Это пример функции, его не нужно отправлять вместе с функцией PrintStats
template <typename InputIt>
int ComputeMedianAge(InputIt range_begin, InputIt range_end) {
  if (range_begin == range_end) {
    return 0;
  }
  vector<typename InputIt::value_type> range_copy(range_begin, range_end);
  auto middle = begin(range_copy) + range_copy.size() / 2;
  nth_element(begin(range_copy), middle, end(range_copy),
              [](const Person &lhs, const Person &rhs) { return lhs.age < rhs.age; });
  return middle->age;
}

void PrintStats(vector<Person> persons) {
  auto females_point =
      partition(begin(persons), end(persons), [](const Person &p) { return p.gender == Gender::MALE; });

  auto unemployed_males_point = partition(begin(persons), females_point, [](const Person &p) { return p.is_employed; });

  auto unemployed_females_point = partition(females_point, end(persons), [](const Person &p) { return p.is_employed; });

  cout << "Median age = " << ComputeMedianAge(begin(persons), end(persons)) << endl
       << "Median age for females = " << ComputeMedianAge(females_point, end(persons)) << endl
       << "Median age for males = " << ComputeMedianAge(begin(persons), females_point) << endl
       << "Median age for employed females = " << ComputeMedianAge(females_point, unemployed_females_point) << endl
       << "Median age for unemployed females = " << ComputeMedianAge(unemployed_females_point, end(persons)) << endl
       << "Median age for employed males = " << ComputeMedianAge(begin(persons), unemployed_males_point) << endl
       << "Median age for unemployed males = " << ComputeMedianAge(unemployed_males_point, females_point) << endl;
}

int main() {
  vector<Person> persons = {
      {31, Gender::MALE, false},   {40, Gender::FEMALE, true}, {24, Gender::MALE, true},    {20, Gender::FEMALE, true},
      {80, Gender::FEMALE, false}, {78, Gender::MALE, false},  {10, Gender::FEMALE, false}, {55, Gender::MALE, true},
  };
  PrintStats(persons);
  return 0;
}
