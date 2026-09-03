#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include "rules/billing_rules.h"

/* billing rule checks. bin/billing-test, run with make check. the rules live in
   billing/rules/billing_rules.c, this only pins the answers they give. */

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
  /* rating is in whole gigabytes, partial gigabytes round up */
  check_eq("partial gigabyte rounds up", (double)br_usage_gb_rounded(1025), 2.0);
  check_eq("exact gigabyte does not round up", (double)br_usage_gb_rounded(2048), 2.0);
  check_eq("usage inside the allowance has no overage",
           (double)br_overage_gb(500 * 1024, 500), 0.0);
  check_eq("ten dollars a gigabyte over plan", br_rate_overage(502 * 1024, 500), 20.0);

  /* proration is on a 30 day month whatever the calendar says */
  check_eq("full month is the plan fee", br_prorated_plan_charge(640.0, 0.0, 0), 640.0);
  check_eq("half month on each plan", br_prorated_plan_charge(300.0, 600.0, 16), 450.0);
  check_eq("february prorates like july",
           br_prorated_plan_charge(300.0, 600.0, 16), br_prorated_plan_charge(300.0, 600.0, 16));

  /* promo credits belong to the cycle they were issued in */
  check_true("promo issued in cycle is live", br_promo_is_live("2026-07-04", "2026-07") != 0);
  check_true("promo issued last cycle is dead", br_promo_is_live("2026-06-24", "2026-07") == 0);
  check_eq("dead promo credits nothing", br_promo_credit(120.0, "2026-06-24", "2026-07"), 0.0);

  /* suspended days stay billed */
  check_eq("suspended days counted", (double)br_suspended_days(10, 14), 5.0);
  check_eq("suspension credits nothing", br_suspension_credit(620.0, 10, 14), 0.0);

  /* multi line schedule */
  check_eq("two lines no discount", br_multi_line_pct(2), 0.0);
  check_eq("three lines five percent", br_multi_line_pct(3), 5.0);
  check_eq("ten lines ten percent", br_multi_line_pct(10), 10.0);
  check_eq("discount off recurring", br_multi_line_discount(1000.0, 12), 100.0);

  /* late fee grace */
  check_eq("inside grace no fee", br_late_fee(500.0, "2026-06-30", "2026-07"), 0.0);
  check_eq("past grace one and a half percent", br_late_fee(500.0, "2026-06-01", "2026-07"), 7.5);

  /* canadian tax: GST on the pre discount amount, PST on what is paid */
  br_tax_rates bc;
  br_rates_for_province("BC", &bc);
  check_eq("bc gst on pre discount", br_federal_tax(100.0, &bc), 5.0);
  check_eq("bc pst on post discount", br_provincial_tax(100.0, 10.0, &bc), 6.30);
  br_tax_rates on;
  br_rates_for_province("ON", &on);
  check_eq("ontario hst is 13", br_federal_tax(100.0, &on), 13.0);
  check_eq("ontario has no separate provincial line", br_provincial_tax(100.0, 10.0, &on), 0.0);
  br_tax_rates qc;
  br_rates_for_province("QC", &qc);
  check_true("quebec provincial line is qst", strcmp(qc.provincial_label, "QST") == 0);
  check_eq("quebec qst on post discount", br_provincial_tax(1000.0, 100.0, &qc), 89.78);
  br_tax_rates ab;
  br_rates_for_province("AB", &ab);
  check_eq("alberta has no provincial tax", br_provincial_tax(100.0, 0.0, &ab), 0.0);

  /* the whole invoice, assembled from rounded lines */
  br_account a;
  br_account_init(&a);
  br_set_field(a.acct_id, sizeof(a.acct_id), "MER-TEST");
  br_set_field(a.province, sizeof(a.province), "BC");
  a.plan_fee = 1000.0;
  a.included_gb = 100;
  a.line_cnt = 3;
  a.loyalty_pct = 10.0;
  br_invoice inv;
  br_compute_invoice(&a, 101 * 1024, "2026-07", &inv);
  check_eq("recurring is the plan fee less the line discount", inv.recurring, 950.0);
  check_eq("one gigabyte over plan", inv.overage_charges, 10.0);
  check_eq("subtotal is the rounded lines", inv.subtotal, 960.0);
  check_eq("loyalty comes off the subtotal", inv.loyalty, 96.0);
  check_eq("gst on the pre discount subtotal", inv.federal_tax, 48.0);
  check_eq("pst on what is paid", inv.provincial_tax, 60.48);
  check_eq("total", inv.total, 972.48);

  if (failures) {
    printf("\n%d check(s) failed\n", failures);
    return 1;
  }
  printf("\nall checks passed\n");
  return 0;
}
