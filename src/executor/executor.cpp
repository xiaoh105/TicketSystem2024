#include <iostream>
#include <sstream>

#include "buffer/buffer_pool_manager.h"
#include "common/stl/pointers.hpp"
#include "executor/executor.h"

#include "user/user_system.h"

bool Parse(string& op, string para[26]) {
  if (std::cin.eof()) {
    return false;
  }
  string command;
  getline(std::cin, command);
  std::stringstream sbuf(command);
  string timestamp;
  sbuf >> timestamp;
  sbuf >> op;
  string tmp;
  while (!sbuf.eof()) {
    sbuf >> tmp;
    sbuf >> para[tmp[1] - 'a'];
  }
  std::cout << timestamp << " ";
  return true;
}

unique_ptr<UserSystem> user_system;

void Initialize() {
  const auto user_buffer = new BufferPoolManager(300, make_unique<DiskManager>("User.dat"));
  user_system = make_unique<UserSystem>(shared_ptr<BufferPoolManager>(user_buffer));
}

void Listen() {
  Initialize();
  string op;
  string para[26];
  while (Parse(op, para)) {
    if (op == "add_user") {
      user_system->AddUser(para);
    } else if (op == "login") {
      user_system->Login(para);
    } else if (op == "logout") {
      user_system->Logout(para);
    } else if (op == "query_profile") {
      user_system->QueryProfile(para);
    } else if (op == "modify_profile") {
      user_system->ModifyProfile(para);
    } else if (op == "exit") {
      std::cout << "bye" << std::endl;
      break;
    } else {
      std::cout << "Operation not supported" << std::endl;
    }
    for (auto & i : para) {
      i.clear();
    }
  }
}

