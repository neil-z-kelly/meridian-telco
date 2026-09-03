#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "rules.h"

/* billing rule checks. bin/billing-test, run with make check.
   every expectation here is the shared spec, billing/rules/BILLING_RULES.md.
   vantage-telco tests/test_billing_rules.py asserts the same numbers. */

using namespace rules;

static int failures = 0;

static void check_eq(const char *what, double got, double want) {
  if (fabs(got - want) > 0.005) {
    printf("FAIL %-52s got %.4f want %.4f\n", what, got, want);
    failures++;
  } else {
    printf("ok   %s\n", what);
  }
}

static void check_true(const char *what, bool cond) {
  if (!cond) {
    printf("FAIL %s\n", what);
    failures++;
  } else {
    printf("ok   %s\n", what);
  }
}

/* pull a scalar out of billing_rules.json by key. good enough for constants;
   the python binding does the same with a real parser. */
static std::string spec_value(const std::string &json, const std::string &key) {
  std::string needle = "\"" + key + "\":";
  size_t at = json.find(needle);
  if (at == std::string::npos) return "";
  at += needle.size();
  while (at < json.size() && json[at] == ' ') at++;
  bool quoted = json[at] == '"';
  if (quoted) at++;
  size_t end = at;
  while (end < json.size() && json[end] != (quoted ? '"' : ',') && json[end] != '}' && json[end] != '\n') end++;
  return json.substr(at, end - at);
}

static void check_spec_constants() {
  std::ifstream in("billing/rules/billing_rules.json");
  std::stringstream buf;
  buf << in.rdbuf();
  std::string json = buf.str();
  check_true("spec file billing/rules/billing_rules.json present", !json.empty());
  check_true("spec version matches binding", spec_value(json, "spec_version") == SPEC_VERSION);
  check_eq("spec BILLING_MONTH_DAYS", atof(spec_value(json, "BILLING_MONTH_DAYS").c_str()), BILLING_MONTH_DAYS);
  check_eq("spec MB_PER_GB", atof(spec_value(json, "MB_PER_GB").c_str()), (double)MB_PER_GB);
  check_eq("spec OVERAGE_RATE_PER_GB", atof(spec_value(json, "OVERAGE_RATE_PER_GB").c_str()), OVERAGE_RATE_PER_GB);
  check_eq("spec MULTI_LINE_TIER1_MIN_LINES", atof(spec_value(json, "MULTI_LINE_TIER1_MIN_LINES").c_str()), MULTI_LINE_TIER1_MIN_LINES);
  check_eq("spec MULTI_LINE_TIER1_PCT", atof(spec_value(json, "MULTI_LINE_TIER1_PCT").c_str()), MULTI_LINE_TIER1_PCT);
  check_eq("spec MULTI_LINE_TIER2_MIN_LINES", atof(spec_value(json, "MULTI_LINE_TIER2_MIN_LINES").c_str()), MULTI_LINE_TIER2_MIN_LINES);
  check_eq("spec MULTI_LINE_TIER2_PCT", atof(spec_value(json, "MULTI_LINE_TIER2_PCT").c_str()), MULTI_LINE_TIER2_PCT);
  check_eq("spec LATE_FEE_GRACE_DAYS", atof(spec_value(json, "LATE_FEE_GRACE_DAYS").c_str()), LATE_FEE_GRACE_DAYS);
  check_eq("spec LATE_FEE_PCT", atof(spec_value(json, "LATE_FEE_PCT").c_str()), LATE_FEE_PCT);
  const char *ambiguous[] = {"PROVINCIAL_TAX_BASE", "PROMO_EXPIRY", "SUSPENSION_CREDIT"};
  for (int i = 0; i < 3; i++) {
    size_t at = json.find(std::string("\"") + ambiguous[i] + "\"");
    std::string status = at == std::string::npos ? "" : spec_value(json.substr(at), "status");
    check_true((std::string("spec flags ") + ambiguous[i] + " RULE_AMBIGUOUS").c_str(), status == "RULE_AMBIGUOUS");
  }
  int n = 0;
  ambiguous_rules(&n);
  check_eq("binding lists three ambiguous rules", (double)n, 3.0);
  check_true("ambiguity flags set", RULE_AMBIGUOUS_PROVINCIAL_TAX_BASE && RULE_AMBIGUOUS_PROMO_EXPIRY && RULE_AMBIGUOUS_SUSPENSION_CREDIT);
}

int main() {
  check_spec_constants();

  /* R1 rounding, half up to the cent, per line */
  check_eq("money rounds half up", money(0.125), 0.13);
  check_eq("money rounds down below half", money(0.1249), 0.12);

  /* R2 proration is on a 30 day month whatever the calendar says */
  check_eq("full month is the plan fee", prorated_plan_charge(640.0, 0.0, 0), 640.0);
  check_eq("half month on each plan", prorated_plan_charge(300.0, 600.0, 16), 450.0);
  check_eq("july and february prorate alike",
           prorated_plan_charge(300.0, 600.0, 16), prorated_plan_charge(300.0, 600.0, 16));
  check_eq("day 31 change clamps to the 30 day month", prorated_plan_charge(310.0, 620.0, 31), 620.0);
  check_eq("each part rounded before the sum", prorated_plan_charge(100.0, 200.0, 8), 46.67 + 76.67);

  /* R3 whole gigabytes, partial gigs round up, $10 a gig */
  check_eq("one extra megabyte is a whole gig", (double)usage_gb_rounded(2000 * 1024 + 1), 2001.0);
  check_eq("overage in gigs", (double)overage_gb(2000 * 1024 + 1000, 2000), 1.0);
  check_eq("ten dollars a gig", rate_overage(2000 * 1024 + 1000, 2000), 10.0);
  check_eq("no overage under allowance", rate_overage(1000, 2000), 0.0);

  /* R9 promo credits belong to the cycle they were issued in (RULE_AMBIGUOUS) */
  check_true("promo issued in cycle is live", promo_is_live("2026-07-04", "2026-07"));
  check_true("promo issued last cycle is dead", !promo_is_live("2026-06-24", "2026-07"));
  check_eq("dead promo credits nothing", promo_credit(120.0, "2026-06-24", "2026-07"), 0.0);
  check_eq("live promo credits its amount", promo_credit(120.0, "2026-07-04", "2026-07"), 120.0);

  /* R10 suspended days stay billed (RULE_AMBIGUOUS) */
  check_eq("suspended days counted", (double)suspended_days(10, 14), 5.0);
  check_eq("suspension credits nothing", suspension_credit(620.0, 10, 14), 0.0);

  /* R4 multi line schedule */
  check_eq("two lines no discount", multi_line_pct(2), 0.0);
  check_eq("three lines five percent", multi_line_pct(3), 5.0);
  check_eq("ten lines ten percent", multi_line_pct(10), 10.0);
  check_eq("discount off recurring", multi_line_discount(1000.0, 12), 100.0);

  /* R5 late fee grace */
  check_eq("inside grace no fee", late_fee(500.0, "2026-06-30", "2026-07"), 0.0);
  check_eq("past grace one and a half percent", late_fee(500.0, "2026-06-01", "2026-07"), 7.5);

  /* R6 loyalty */
  check_eq("loyalty off the subtotal", loyalty_discount(1000.0, 8.0), 80.0);

  /* R7 / R8 canadian tax: GST on the pre discount amount, PST on what is paid */
  TaxRates bc = rates_for_province("BC");
  check_eq("bc gst on pre discount", federal_tax(100.0, bc), 5.0);
  check_eq("bc pst on post discount", provincial_tax(100.0, 10.0, bc), 6.30);
  TaxRates on = rates_for_province("ON");
  check_eq("ontario hst is 13", federal_tax(100.0, on), 13.0);
  check_eq("ontario has no separate provincial line", provincial_tax(100.0, 10.0, on), 0.0);
  TaxRates qc = rates_for_province("QC");
  check_true("quebec provincial line is qst", qc.provincial_label == "QST");
  check_eq("quebec qst on post discount", provincial_tax(1000.0, 100.0, qc), 89.78);
  TaxRates ab = rates_for_province("AB");
  check_eq("alberta has no provincial tax", provincial_tax(100.0, 0.0, ab), 0.0);
  check_eq("unknown province is gst only", rates_for_province("YT").provincial_pct, 0.0);

  if (failures) {
    printf("\n%d check(s) failed\n", failures);
    return 1;
  }
  printf("\nall checks passed\n");
  return 0;
}
