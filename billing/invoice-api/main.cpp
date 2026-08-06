#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../inventory-api/httpd.h"
#include "../accounts.h"
#include "../rating.h"
#include "../discounts.h"

/* invoice api. GET /invoices?acct=BEACON-004417[&period=2026-07]
   GET /invoices returns every account for the periods we have usage for. */

static std::vector<Account> g_accts;
static std::vector<UsageRec> g_usage;

static std::string invoice_json(const Account &a, const UsageRec &u) {
  double charges = rate_overage(u.usage_mb, a.included_gb);
  double discounted = apply_loyalty(charges, a.loyalty_pct);
  double total = apply_tax(discounted, a.tax_pct);
  char buf[1400];
  snprintf(buf, sizeof(buf),
           "{\"ACCT_ID\":\"%s\",\"CUST_NM\":\"%s\",\"TAX_ID\":\"%s\",\"SVC_ADDR\":\"%s\","
           "\"PERIOD\":\"%s\",\"USAGE_MB\":%ld,\"USAGE_GB_RATED\":%ld,\"INCLUDED_GB\":%ld,"
           "\"OVERAGE_GB\":%ld,\"OVERAGE_CHARGES\":%.2f,\"LOYALTY_PCT\":%.2f,"
           "\"AFTER_DISCOUNT\":%.2f,\"TAX_PCT\":%.2f,\"INVOICE_TOTAL\":%.2f}",
           a.acct_id.c_str(), json_escape(a.cust_nm).c_str(), a.tax_id.c_str(),
           json_escape(a.svc_addr).c_str(), u.period.c_str(), u.usage_mb,
           usage_gb_rounded(u.usage_mb), a.included_gb,
           overage_gb(u.usage_mb, a.included_gb), charges, a.loyalty_pct,
           discounted, a.tax_pct, total);
  return buf;
}

static std::string handle_invoices(const HttpRequest &req, int *status) {
  std::map<std::string, std::string> q = req.query;
  std::string want_acct = q.count("acct") ? q["acct"] : "";
  std::string want_period = q.count("period") ? q["period"] : "";
  std::string out = "{\"invoices\":[";
  int n = 0;
  for (size_t i = 0; i < g_usage.size(); i++) {
    UsageRec u = g_usage[i];
    if (!want_acct.empty() && u.acct_id != want_acct) continue;
    if (!want_period.empty() && u.period != want_period) continue;
    Account a;
    if (!find_account(g_accts, u.acct_id, &a)) continue;
    if (n) out += ",";
    out += invoice_json(a, u);
    n++;
  }
  out += "]}";
  if (n == 0 && !want_acct.empty()) *status = 404;
  return out;
}

int main(int argc, char **argv) {
  std::string accts_dir = "billing/accounts";
  std::string usage_csv = "data/usage.csv";
  int port = 8082;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--accounts" && i + 1 < argc) accts_dir = argv[++i];
    else if (a == "--usage" && i + 1 < argc) usage_csv = argv[++i];
    else if (a == "--port" && i + 1 < argc) port = atoi(argv[++i]);
  }
  g_accts = load_accounts(accts_dir);
  g_usage = load_usage(usage_csv);
  if (g_accts.empty()) {
    fprintf(stderr, "no account records under %s\n", accts_dir.c_str());
    return 1;
  }
  fprintf(stderr, "loaded %d accounts, %d usage records\n", (int)g_accts.size(), (int)g_usage.size());
  std::vector<Route> routes;
  Route r; r.prefix = "/invoices"; r.fn = handle_invoices; routes.push_back(r);
  return serve(port, routes);
}
