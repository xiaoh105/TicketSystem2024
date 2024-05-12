#include <iostream>
#include <map>
#include "common/utils.h"
#include "storage/index/b_plus_tree.h"

int main() {
  BPlusTree<pair<unsigned long long, int>, int, std::less<>> mp(make_shared<BufferPoolManager>(256, make_unique<DiskManager>("test.txt")), std::less<>());
  int n;
  std::cin >> n;
  for (int i = 1; i <= n; ++i) {
    std::string op;
    std::cin >> op;
    if (op == "insert") {
      std::string name;
      int val;
      std::cin >> name >> val;
      mp.Insert(make_pair(StringHash(name), val), val);
    } else if (op == "delete") {
      std::string name;
      int val;
      std::cin >> name >> val;
      mp.Remove(make_pair(StringHash(name), val));
    } else {
      assert(op == "find");
      std::string name;
      std::cin >> name;
      auto h = StringHash(name);
      auto it = mp.LowerBound(make_pair(h, INT32_MIN));
      if (!it) {
        std::cout << "null" << std::endl;
        continue;
      }
      vector<int> ret{};
      while ((*it).first.first == h) {
        ret.push_back((*it).first.second);
        if (it.IsEnd()) {
          break;
        }
        ++it;
      }
      if (ret.empty()) {
        std::cout << "null" << std::endl;
      } else {
        for (auto id : ret) {
          std::cout << id << " ";
        }
        std::cout << std::endl;
      }
    }
  }
}