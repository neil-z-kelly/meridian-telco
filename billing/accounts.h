#ifndef MERIDIAN_ACCOUNTS_H
#define MERIDIAN_ACCOUNTS_H

#include <string>
#include <vector>
#include <map>

struct Account {
  std::string acct_id;
  std::string cust_nm;
  std::string tax_id;
  std::string svc_addr;
  std::string plan_cd;
  long included_gb;
  double loyalty_pct;
  double tax_pct;
};

std::vector<Account> load_accounts(const std::string &dir);
bool find_account(const std::vector<Account> &accts, const std::string &id, Account *out);

struct UsageRec {
  std::string acct_id;
  std::string period;
  long usage_mb;
};

std::vector<UsageRec> load_usage(const std::string &csv_path);

#endif
