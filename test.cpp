#include "kod_array.h"

int main() {
//kod::array<int> a{4}; // init with capacity
  kod::array<int> a;
  
  a.emplace_back(1);
  a.emplace(0, 2);
  
  int x = 3;
  a.push_back(x);
  a.push(0, x);

  a.removeAt(0);
  a.remove(a.back());
  
  int y = a.pop_back();
}
