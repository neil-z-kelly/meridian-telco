#include <cmath>
#include <cstdio>
#include <cstring>

#include "../rules/telco_rules.h"

/* consolidated rule checks. bin/shared-rules-test, run with make check.

   one check per rule that the two estates used to disagree on. each one is
   written so that it fails against the behaviour vantage-telco used to run,
   and the old vantage answer is quoted beside it. the two rules the estates
   already agreed on are checked as matches, not as fixes. */

static int failures = 0;

static void check_eq(const char *what, double got, double want) {
  if (fabs(got - want) > 0.005) {
    printf("FAIL %-58s got %.4f want %.4f\n", what, got, want);
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
  /* overage: whole gigabytes, partial gigs round up, $10 a gig.
     one megabyte over a 1000GB allowance is a whole billable gigabyte.
     vantage used to rate this to the exact megabyte and charge $0.012. */
  check_eq("one MB over the allowance bills a whole GB",
           tr_rate_overage(1024L * 1000L + 1L, 1000), 10.00);
  check_eq("usage inside the allowance is not rated", tr_rate_overage(1000, 2000), 0.00);

  /* proration: 30 day month whatever the calendar says. a change on the 31st
     leaves nothing on the new plan. vantage used to answer 609.68 here. */
  check_eq("change on the 31st is a full month of the old plan",
           tr_prorated_plan_charge(310.0, 620.0, 31), 620.00);
  check_eq("february and july prorate alike", tr_prorated_plan_charge(300.0, 600.0, 16),
           450.00);

  /* promo credits expire with the cycle they were issued in. a credit issued
     on 2026-06-24 is dead in july. vantage used to keep it live for 30 days. */
  check_true("promo issued last cycle is dead", !tr_promo_is_live("2026-06-24", "2026-07"));
  check_eq("dead promo credits nothing", tr_promo_credit(350.0, "2026-06-24", "2026-07"), 0.00);
  check_eq("promo issued in cycle credits in full",
           tr_promo_credit(350.0, "2026-07-04", "2026-07"), 350.00);

  /* a suspended line is billed the full month. vantage used to credit
     620 / 31 * 5 = 100.00 for the same five days. */
  check_eq("five suspended days counted", (double)tr_suspended_days(10, 14), 5.0);
  check_eq("suspension credits nothing", tr_suspension_credit(620.0, 10, 14), 0.00);

  /* rounding: each line is rounded to cents as it is produced and the total is
     their sum. vantage used to carry full precision and round once at the end,
     so it answered 0.03 where the register answers 0.04. */
  check_eq("each line rounds to cents", tr_money(0.005) + tr_money(0.005) + tr_money(0.015),
           0.04);

  /* multi line discount: already identical in both estates. */
  check_eq("two lines no discount", tr_multi_line_pct(2), 0.0);
  check_eq("three lines five percent", tr_multi_line_pct(3), 5.0);
  check_eq("ten lines ten percent", tr_multi_line_pct(10), 10.0);
  check_eq("discount off recurring", tr_multi_line_discount(1000.0, 12), 100.00);

  /* late fee grace: already identical in both estates. */
  check_eq("inside grace no fee", tr_late_fee(500.0, "2026-06-30", "2026-07"), 0.00);
  check_eq("past grace one and a half percent", tr_late_fee(500.0, "2026-06-01", "2026-07"),
           7.50);

  /* tax. the federal component is settled: GST and HST sit on the pre discount
     amount in both estates. */
  TrTaxRates bc = tr_rates_for_province("BC");
  TrTaxRates on = tr_rates_for_province("ON");
  TrTaxRates qc = tr_rates_for_province("QC");
  TrTaxRates ab = tr_rates_for_province("AB");
  check_eq("bc gst on the pre discount amount", tr_federal_tax(100.0, bc), 5.00);
  check_eq("ontario hst is 13", tr_federal_tax(100.0, on), 13.00);
  check_eq("alberta has no provincial line",
           tr_provincial_tax(100.0, 0.0, ab, TR_TAX_BASE_POST_DISCOUNT), 0.00);
  check_eq("harmonized provinces have no provincial line",
           tr_provincial_tax(100.0, 10.0, on, TR_TAX_BASE_POST_DISCOUNT), 0.00);
  check_true("quebec provincial line is qst", strcmp(qc.provincial_label, "QST") == 0);

  /* the provincial tax base is the open question. both bases are implemented,
     neither is the default, and each estate names the one it runs on. */
  check_eq("post discount base is what meridian runs",
           tr_provincial_tax(100.0, 10.0, bc, TR_TAX_BASE_POST_DISCOUNT), 6.30);
  check_eq("pre discount base is what vantage runs",
           tr_provincial_tax(100.0, 10.0, bc, TR_TAX_BASE_PRE_DISCOUNT), 7.00);
  check_eq("quebec post discount base",
           tr_provincial_tax(1000.0, 100.0, qc, TR_TAX_BASE_POST_DISCOUNT), 89.78);
  check_eq("quebec pre discount base",
           tr_provincial_tax(1000.0, 100.0, qc, TR_TAX_BASE_PRE_DISCOUNT), 99.75);

  /* the whole invoice, on the account the demo is built around: TN-0001,
     Beacon Manufacturing Corp, BC, meridian BEACON-004417. */
  TrInvoiceInput in;
  in.province = "BC";
  in.period = "2026-07";
  in.plan_fee = 4200.0;
  in.prev_plan_fee = 2400.0;
  in.plan_chg_day = 4;
  in.included_gb = 5000;
  in.line_cnt = 14;
  in.usage_mb = 7472103;
  in.promo_amt = 350.0;
  in.promo_dt = "2026-06-24";
  in.susp_start = 0;
  in.susp_end = 0;
  in.prior_bal = 700.9;
  in.prior_due = "2026-06-30";
  in.loyalty_pct = 4.0;

  TrInvoice legacy;
  tr_compute_invoice(&in, tr_policy_legacy(), &legacy);
  check_eq("TN-0001 subtotal is assembled from rounded lines", legacy.subtotal,
           legacy.recurring + legacy.overage_charges + legacy.late_fee -
               legacy.suspension_credit - legacy.promo_credit);
  check_eq("TN-0001 promo credit is dead in july", legacy.promo_credit, 0.00);
  check_eq("TN-0001 total on the post discount provincial base", legacy.total, 28640.59);

  TrPolicy pre = tr_policy_legacy();
  pre.provincial_tax_base = TR_TAX_BASE_PRE_DISCOUNT;
  TrInvoice alternate;
  tr_compute_invoice(&in, pre, &alternate);
  check_true("the tax base is the only thing the two policies move",
             alternate.subtotal == legacy.subtotal && alternate.total > legacy.total);

  if (failures) {
    printf("\n%d consolidated rule check(s) failed\n", failures);
    return 1;
  }
  printf("\nall consolidated rule checks passed\n");
  return 0;
}
