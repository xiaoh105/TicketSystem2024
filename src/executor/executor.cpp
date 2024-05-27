#include <iostream>
#include <sstream>

#include "buffer/buffer_pool_manager.h"
#include "common/stl/pointers.hpp"
#include "executor/executor.h"

#include "user/user_system.h"
#include "ticket/train_system.h"

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

shared_ptr<UserSystem> user_system;
shared_ptr<TrainSystem> ticket_system;

void Initialize() {
  const auto user_buffer = new BufferPoolManager(200, make_unique<DiskManager>("user.dat"));
  user_system = make_shared<UserSystem>(shared_ptr(user_buffer));
  const auto train_buffer = new BufferPoolManager(500, make_unique<DiskManager>("train.dat"));
  const auto station_buffer = new BufferPoolManager(250, make_unique<DiskManager>("station.dat"));
  const auto waitlist_buffer = new BufferPoolManager(200, make_unique<DiskManager>("waitlist.dat"));
  const auto orderlist_buffer = new BufferPoolManager(200, make_unique<DiskManager>("orderlist.dat"));
  const auto ticket_buffer = new BufferPoolManager(200, make_unique<DiskManager>("ticket.dat"));
  ticket_system = make_shared<TrainSystem>(shared_ptr(train_buffer), shared_ptr(station_buffer),
                                           shared_ptr(ticket_buffer), shared_ptr(waitlist_buffer),
                                           shared_ptr(orderlist_buffer));
}

void Listen() {
  std::ios::sync_with_stdio(false);
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
    } else if (op == "add_train") {
      ticket_system->AddTrain(para);
    } else if (op == "delete_train") {
      ticket_system->DeleteTrain(para);
    } else if (op == "release_train") {
      ticket_system->ReleaseTrain(para);
    } else if (op == "query_train") {
      ticket_system->QueryTrain(para);
    } else if (op == "query_ticket") {
      ticket_system->QueryTicket(para);
    } else if (op == "query_transfer") {
      ticket_system->QueryTransfer(para);
    } else if (op == "buy_ticket") {
      ticket_system->BuyTicket(para, user_system);
    } else if (op == "query_order") {
      ticket_system->QueryOrder(para, user_system);
    } else if (op == "refund_ticket") {
      ticket_system->RefundTicket(para, user_system);
    } else {
      std::cout << "Operation not supported" << std::endl;
    }
    for (auto & i : para) {
      i.clear();
    }
    op.clear();
  }
}

