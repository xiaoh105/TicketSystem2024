#include <iostream>
#include <map>
#include "storage/index/b_plus_tree.h"

int main() {
  BPlusTree<int, int, std::less<>> mp_src(make_unique<BufferPoolManager>(256, make_unique<DiskManager>("test.txt")), std::less<>());
  map<int, int> mp;
  for (int i = 1; i <= 100000; ++i) {
    int op = rand() % 3;
    int x = rand();
    vector<int> res{};
    if (op == 0) {
      mp_src.GetValue(x, &res);
      assert(!(mp.find(x) != mp.end() ^ !res.empty()));
    } else if (op == 1) {
      int y = rand();
      if (mp.find(x) == mp.end()) {
        mp[x] = y;
        mp_src.Insert(x, y);
      }
    } else if (op == 2) {
      mp_src.Remove(x);
      if (mp.find(x) != mp.end()) {
        mp.erase(x);
      }
    }
  }
}