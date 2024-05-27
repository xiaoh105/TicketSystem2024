#pragma once
#include <cstdint>
#include "buffer/buffer_pool_manager.h"
#include "storage/index/b_plus_tree.h"
#include "common/rid.h"

struct UserProfile {
  char username_[21]{};
  char password_[31]{};
  char name_[21]{};
  char mail_addr_[31]{};
  int8_t login_info_;
  int8_t privilege_{-1};
};

std::ostream &operator<<(std::ostream &os, const UserProfile &val);

class UserSystem {
public:
  UserSystem(shared_ptr<BufferPoolManager> bpm); //NOLINT
  ~UserSystem();
  void AddUser(std::string para[26]);
  void Login(std::string para[26]);
  void Logout(std::string para[26]);
  void QueryProfile(std::string para[26]);
  void ModifyProfile(std::string para[26]);
  bool LoginStatus(const std::string &username);

private:
  bool GetProfile(const std::string &username, UserProfile &profile) const;
  shared_ptr<BufferPoolManager> bpm_;
  unique_ptr<BPlusTree<unsigned long long, RID, std::less<>>> index_;
  page_id_t tuple_page_id_;
  page_id_t login_timestamp_;
};