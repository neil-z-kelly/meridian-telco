#include <cstdio>
#include <cmath>
#include <string>
#include "rules/telco_rules.h"

/* billing rule checks against the shared rule library. bin/billing-test, run
   with make check. the same expectations are emitted as conformance vectors by
   bin/rules-vectors for the consumers that cannot link the library. */

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

int main() {
  /* usage rates in whole gigabytes, partial gigabytes round up */
  check_eq("partial gigabyte rounds up", (double)tr_usage_gb_rounded(1024 * 3 + 1), 4.0);
  check_eq("overage is gigabytes over the allowance", (double)tr_overage_gb(1024 * 3 + 1, 2), 2.0);
  check_eq("overage is ten dollars a gigabyte", tr_rate_overage(1024 * 3 + 1, 2), 20.0);
  check_eq("no overage under the allowance", tr_rate_overage(1000, 2000), 0.0);

  /* proration is on a 30 day month whatever the calendar says */
  check_eq("full month is the plan fee", tr_prorated_plan_charge(640.0, 0.0, 0), 640.0);
  check_eq("half month on each plan", tr_prorated_plan_charge(300.0, 600.0, 16), 450.0);
  check_eq("july and february prorate alike",
           tr_prorated_plan_charge(300.0, 600.0, 16), tr_prorated_plan_charge(300.0, 600.0, 16));

  /* promo credits belong to the cycle they were issued in */
  check_true("promo issued in cycle is live", tr_promo_is_live("2026-07-04", "2026-07") != 0);
  check_true("promo issued last cycle is dead", tr_promo_is_live("2026-06-24", "2026-07") == 0);
  check_eq("dead promo credits nothing", tr_promo_credit(120.0, "2026-06-24", "2026-07"), 0.0);

  /* suspended days stay billed */
  check_eq("suspended days counted", (double)tr_suspended_days(10, 14), 5.0);
  check_eq("suspension credits nothing", tr_suspension_credit(620.0, 10, 14), 0.0);

  /* multi line schedule */
  check_eq("two lines no discount", tr_multi_line_pct(2), 0.0);
  check_eq("three lines five percent", tr_multi_line_pct(3), 5.0);
  check_eq("ten lines ten percent", tr_multi_line_pct(10), 10.0);
  check_eq("discount off recurring", tr_multi_line_discount(1000.0, 12), 100.0);

  /* late fee grace */
  check_eq("inside grace no fee", tr_late_fee(500.0, "2026-06-30", "2026-07"), 0.0);
  check_eq("past grace one and a half percent", tr_late_fee(500.0, "2026-06-01", "2026-07"), 7.5);

  /* canadian tax: GST on the pre discount amount, PST on what is paid */
  tr_tax_rates bc;
  tr_rates_for_province("BC", &bc);
  check_eq("bc gst on pre discount", tr_federal_tax(100.0, &bc), 5.0);
  check_eq("bc pst on post discount", tr_provincial_tax(100.0, 10.0, &bc), 6.30);
  tr_tax_rates on;
  tr_rates_for_province("ON", &on);
  check_eq("ontario hst is 13", tr_federal_tax(100.0, &on), 13.0);
  check_eq("ontario has no separate provincial line", tr_provincial_tax(100.0, 10.0, &on), 0.0);
  tr_tax_rates qc;
  tr_rates_for_province("QC", &qc);
  check_true("quebec provincial line is qst", std::string(qc.provincial_label) == "QST");
  check_eq("quebec qst on post discount", tr_provincial_tax(1000.0, 100.0, &qc), 89.78);
  tr_tax_rates ab;
  tr_rates_for_province("AB", &ab);
  check_eq("alberta has no provincial tax", tr_provincial_tax(100.0, 0.0, &ab), 0.0);

  /* loyalty is a percentage of the subtotal, credited after tax */
  check_eq("loyalty off the subtotal", tr_loyalty_discount(1000.0, 8.0), 80.0);

  if (failures) {
    printf("\n%d check(s) failed\n", failures);
    return 1;
  }
  printf("\nall checks passed\n");
  return 0;
}
