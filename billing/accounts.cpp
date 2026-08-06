#include "accounts.h"
#include "../mediation/csv.h"
#include <dirent.h>
#include <fstream>
#include <cstdlib>

/* account master. one flat record per account under billing/accounts/.
   there was a migration to the store planned. */

static Account parse_rec(const std::string &path) {
  Account a;
  a.included_gb = 0;
  a.loyalty_pct = 0.0;
  a.tax_pct = 0.0;
  std::ifstream f(path.c_str());
  std::string line;
  while (std::getline(f, line)) {
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string k = line.substr(0, eq);
    std::string v = line.substr(eq + 1);
    if (k == "ACCT_ID") a.acct_id = v;
    else if (k == "CUST_NM") a.cust_nm = v;
    else if (k == "TAX_ID") a.tax_id = v;
    else if (k == "SVC_ADDR") a.svc_addr = v;
    else if (k == "PLAN_CD") a.plan_cd = v;
    else if (k == "INCLUDED_GB") a.included_gb = atol(v.c_str());
    else if (k == "LOYALTY_PCT") a.loyalty_pct = atof(v.c_str());
    else if (k == "TAX_PCT") a.tax_pct = atof(v.c_str());
  }
  return a;
}

std::vector<Account> load_accounts(const std::string &dir) {
  std::vector<Account> out;
  DIR *d = opendir(dir.c_str());
  if (!d) return out;
  struct dirent *e;
  while ((e = readdir(d)) != 0) {
    std::string nm = e->d_name;
    if (nm.size() < 5) continue;
    if (nm.substr(nm.size() - 4) != ".rec") continue;
    out.push_back(parse_rec(dir + "/" + nm));
  }
  closedir(d);
  return out;
}

bool find_account(const std::vector<Account> &accts, const std::string &id, Account *out) {
  for (size_t i = 0; i < accts.size(); i++) {
    if (accts[i].acct_id == id) { *out = accts[i]; return true; }
  }
  return false;
}

std::vector<UsageRec> load_usage(const std::string &csv_path) {
  std::vector<UsageRec> out;
  std::vector<Row> rows = read_csv(csv_path);
  for (size_t i = 0; i < rows.size(); i++) {
    Row r = rows[i];
    UsageRec u;
    u.acct_id = r["ACCT_ID"];
    u.period = r["PERIOD"];
    u.usage_mb = atol(r["USAGE_MB"].c_str());
    out.push_back(u);
  }
  return out;
}
