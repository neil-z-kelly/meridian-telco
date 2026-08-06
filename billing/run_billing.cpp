#include <cstdio>
#include <cstdlib>
#include <string>
#include "accounts.h"
#include "rating.h"
#include "discounts.h"

/* monthly billing run. prints the invoice register. */

int main(int argc, char **argv) {
  std::string accts_dir = "billing/accounts";
  std::string usage_csv = "data/usage.csv";
  std::string period = "";
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--accounts" && i + 1 < argc) accts_dir = argv[++i];
    else if (a == "--usage" && i + 1 < argc) usage_csv = argv[++i];
    else if (a == "--period" && i + 1 < argc) period = argv[++i];
  }
  std::vector<Account> accts = load_accounts(accts_dir);
  std::vector<UsageRec> usage = load_usage(usage_csv);
  double grand = 0.0;
  printf("%-16s %-8s %12s %10s %12s\n", "ACCT_ID", "PERIOD", "USAGE_MB", "OVER_GB", "TOTAL");
  for (size_t i = 0; i < usage.size(); i++) {
    UsageRec u = usage[i];
    if (!period.empty() && u.period != period) continue;
    Account a;
    if (!find_account(accts, u.acct_id, &a)) {
      printf("%-16s %-8s %12ld %10s %12s\n", u.acct_id.c_str(), u.period.c_str(),
             u.usage_mb, "-", "NO ACCOUNT");
      continue;
    }
    double total = invoice_total(rate_overage(u.usage_mb, a.included_gb),
                                 a.loyalty_pct, a.tax_pct);
    grand += total;
    printf("%-16s %-8s %12ld %10ld %12.2f\n", u.acct_id.c_str(), u.period.c_str(),
           u.usage_mb, overage_gb(u.usage_mb, a.included_gb), total);
  }
  printf("%-16s %-8s %12s %10s %12.2f\n", "TOTAL", "", "", "", grand);
  return 0;
}
