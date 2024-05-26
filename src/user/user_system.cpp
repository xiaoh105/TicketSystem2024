#include "user/user_system.h"

#include "common/utils.h"
#include "storage/page/tuple_page.h"

using std::string;
using std::ostream, std::cout, std::endl;
ostream &operator<<(ostream &os, const UserProfile &val) {
  os << val.username_ << " " << val.name_ << " "
     << val.mail_addr_ << " " << static_cast<int>(val.privilege_);
  return os;
}

UserSystem::UserSystem(shared_ptr<BufferPoolManager> bpm)
: bpm_(std::move(bpm)),
  index_(new BPlusTree<unsigned long long, RID, std::less<>>(bpm_, std::less<>())) {
  auto cur_guard = bpm_->FetchPageRead(0);
  auto cur_page = cur_guard.As<BPlusTreeHeaderPage>();
  tuple_page_id_ = cur_page->tuple_page_id_;
}

UserSystem::~UserSystem() {
  auto cur_guard = bpm_->FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->tuple_page_id_ = tuple_page_id_;
}

void UserSystem::Login(string para[26]) {
  string &cur_user = para['u' - 'a'];
  string &password = para['p' - 'a'];
  if (login_status_.find(cur_user) != login_status_.end()) {
    Fail();
    return;
  }
  UserProfile cur_profile;
  if (GetProfile(cur_user, cur_profile)) {
    if (strcmp(cur_profile.password_, password.c_str()) == 0) {
      login_status_[cur_user] = cur_profile.privilege_;
      Succeed();
    } else {
      Fail();
    }
  } else {
    Fail();
  }
}

void UserSystem::Logout(std::string para[26]) {
  string &cur_user_ = para['u' - 'a'];
  auto it = login_status_.find(cur_user_);
  if (it != login_status_.end()) {
    login_status_.erase(it);
    Succeed();
  } else {
    Fail();
  }
}

void UserSystem::AddUser(std::string para[26]) {
  const string &cur_username = para['c' - 'a'];
  const string &username = para['u' - 'a'];
  const string &password = para['p' - 'a'];
  const string &name = para['n' - 'a'];
  const string &mail = para['m' - 'a'];
  auto privilege = static_cast<int8_t>(stoi(para['g' - 'a']));
  bool is_first = (tuple_page_id_ == INVALID_PAGE_ID);
  if (!is_first && login_status_[cur_username] <= privilege) {
    Fail();
    return;
  }
  if (!is_first && login_status_.find(cur_username) == login_status_.end()) {
    Fail();
    return;
  }
  if (is_first) {
    privilege = 10;
  }
  vector<RID> tmp;
  index_->GetValue(StringHash(username), &tmp);
  if (!tmp.empty()) {
    Fail();
    return;
  }
  UserProfile data;
  username.copy(data.username_, string::npos);
  password.copy(data.password_, string::npos);
  name.copy(data.name_, string::npos);
  mail.copy(data.mail_addr_, string::npos);
  data.privilege_ = privilege;
  if (tuple_page_id_ == INVALID_PAGE_ID) {
    auto cur_guard = bpm_->NewPageGuarded(&tuple_page_id_);
    auto cur_page = cur_guard.AsMut<TuplePage<UserProfile>>();
    auto pos = cur_page->Append(data);
    index_->Insert(StringHash(username), {tuple_page_id_, pos});
  } else {
    auto cur_guard = bpm_->FetchPageWrite(tuple_page_id_);
    auto cur_page = cur_guard.AsMut<TuplePage<UserProfile>>();
    if (cur_page->Full()) {
      auto new_guard = bpm_->NewPageGuarded(&tuple_page_id_);
      cur_page = new_guard.AsMut<TuplePage<UserProfile>>();
    }
    auto pos = cur_page->Append(data);
    index_->Insert(StringHash(username), {tuple_page_id_, pos});
  }
  Succeed();
}

void UserSystem::ModifyProfile(std::string para[26]) {
  string &cur_username = para['c' - 'a'];
  string &username = para['u' - 'a'];
  auto it = login_status_.find(cur_username);
  if (it == login_status_.end()) {
    Fail();
    return;
  }
  vector<RID> user_rid;
  index_->GetValue(StringHash(username), &user_rid);
  if (user_rid.empty()) {
    Fail();
    return;
  }
  auto cur_guard = bpm_->FetchPageWrite(user_rid[0].page_id_);
  auto cur_page = cur_guard.AsMut<TuplePage<UserProfile>>();
  UserProfile &data = cur_page->operator[](user_rid[0].pos_);
  if (data.privilege_ >= it->second && cur_username != username) {
    Fail();
    return;
  }
  const string &privilege = para['g' - 'a'];
  const string &password = para['p' - 'a'];
  const string &name = para['n' - 'a'];
  const string &mail = para['m' - 'a'];
  if (!privilege.empty() && std::stoi(privilege) >= it->second) {
    Fail();
    return;
  }
  if (!privilege.empty()) {
    data.privilege_ = static_cast<int8_t>(std::stoi(privilege));
  }
  if (!password.empty()) {
    memset(data.password_, 0, sizeof(data.password_));
    password.copy(data.password_, string::npos);
  }
  if (!name.empty()) {
    memset(data.name_, 0, sizeof(data.name_));
    name.copy(data.name_, string::npos);
  }
  if (!mail.empty()) {
    memset(data.mail_addr_, 0, sizeof(data.mail_addr_));
    mail.copy(data.mail_addr_, string::npos);
  }
  cout << data << endl;
}

void UserSystem::QueryProfile(std::string para[26]) {
  string cur_username = para['c' - 'a'];
  string username = para['u' - 'a'];
  auto it = login_status_.find(cur_username);
  if (it == login_status_.end()) {
    Fail();
    return;
  }
  UserProfile data;
  if (!GetProfile(username, data)) {
    Fail();
    return;
  }
  if (data.privilege_ >= it->second && username != cur_username) {
    Fail();
    return;
  }
  cout << data << endl;
}

bool UserSystem::GetProfile(const std::string &username, UserProfile &profile) const {
  vector<RID> user_rid;
  index_->GetValue(StringHash(username), &user_rid);
  if (user_rid.empty()) {
    return false;
  }
  auto cur_guard = bpm_->FetchPageRead(user_rid[0].page_id_);
  auto cur_page = cur_guard.As<TuplePage<UserProfile>>();
  profile = cur_page->At(user_rid[0].pos_);
  return true;
}
