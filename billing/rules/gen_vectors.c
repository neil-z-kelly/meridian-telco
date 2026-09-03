#include "telco_rules.h"

#include <stdio.h>
#include <string.h>

/* Emits the shared conformance vectors as JSON on stdout.

   The vectors are the contract every estate is held to: meridian and vantage
   both run the rules out of telco_rules.c, and the java report module, which
   cannot link it, is tested against this file. Regenerate with
   `make vectors`; `make check` fails when the checked in copy is stale. */

static int first = 1;

static void open_vector(const char *kind, const char *id) {
  if (!first) printf(",\n");
  first = 0;
  printf("    {\"kind\": \"%s\", \"id\": \"%s\"", kind, id);
}

static void close_vector(void) {
  printf("}");
}

static void money_vector(const char *id, double amount) {
  open_vector("money", id);
  printf(", \"in\": {\"amount\": %.6f}, \"out\": {\"value\": %.2f}", amount, tr_money(amount));
  close_vector();
}

static void gb_vector(const char *id, long usage_mb) {
  open_vector("usage_gb_rounded", id);
  printf(", \"in\": {\"usage_mb\": %ld}, \"out\": {\"gb\": %ld}", usage_mb,
         tr_usage_gb_rounded(usage_mb));
  close_vector();
}

static void overage_vector(const char *id, long usage_mb, long included_gb) {
  open_vector("overage_gb", id);
  printf(", \"in\": {\"usage_mb\": %ld, \"included_gb\": %ld}, \"out\": {\"gb\": %ld}",
         usage_mb, included_gb, tr_overage_gb(usage_mb, included_gb));
  close_vector();
  open_vector("rate_overage", id);
  printf(", \"in\": {\"usage_mb\": %ld, \"included_gb\": %ld}, \"out\": {\"amount\": %.2f}",
         usage_mb, included_gb, tr_rate_overage(usage_mb, included_gb));
  close_vector();
}

static void proration_vector(const char *id, double fee, double prev_fee, int change_day,
                             const char *period) {
  open_vector("prorated_plan_charge", id);
  printf(", \"in\": {\"monthly_fee\": %.2f, \"prev_monthly_fee\": %.2f, "
         "\"change_day\": %d, \"period\": \"%s\"}, \"out\": {\"amount\": %.2f}",
         fee, prev_fee, change_day, period,
         tr_prorated_plan_charge(fee, prev_fee, change_day, period));
  close_vector();
}

static void lines_vector(const char *id, double recurring, int line_count) {
  open_vector("multi_line_discount", id);
  printf(", \"in\": {\"recurring_charge\": %.2f, \"line_count\": %d}, "
         "\"out\": {\"pct\": %.3f, \"amount\": %.2f}",
         recurring, line_count, tr_multi_line_pct(line_count),
         tr_multi_line_discount(recurring, line_count));
  close_vector();
}

static void promo_vector(const char *id, double amount, const char *issued_on,
                         const char *period) {
  open_vector("promo_credit", id);
  printf(", \"in\": {\"amount\": %.2f, \"issued_on\": \"%s\", \"period\": \"%s\"}, "
         "\"out\": {\"live\": %s, \"amount\": %.2f}",
         amount, issued_on, period, tr_promo_is_live(issued_on, period) ? "true" : "false",
         tr_promo_credit(amount, issued_on, period));
  close_vector();
}

static void suspension_vector(const char *id, double fee, int start_day, int end_day,
                              const char *period) {
  open_vector("suspension_credit", id);
  printf(", \"in\": {\"monthly_fee\": %.2f, \"start_day\": %d, \"end_day\": %d, "
         "\"period\": \"%s\"}, \"out\": {\"days\": %d, \"amount\": %.2f}",
         fee, start_day, end_day, period, tr_suspended_days(start_day, end_day),
         tr_suspension_credit(fee, start_day, end_day, period));
  close_vector();
}

static void latefee_vector(const char *id, double balance, const char *due_date,
                           const char *period) {
  open_vector("late_fee", id);
  printf(", \"in\": {\"prior_balance\": %.2f, \"due_date\": \"%s\", \"period\": \"%s\"}, "
         "\"out\": {\"days_past_due\": %ld, \"amount\": %.2f}",
         balance, due_date, period, tr_days_past_due(due_date, period),
         tr_late_fee(balance, due_date, period));
  close_vector();
}

static void tax_vector(const char *id, const char *province, double amount, double discount) {
  TrTaxRates rates;
  tr_rates_for_province(province, &rates);
  open_vector("tax", id);
  printf(", \"in\": {\"province\": \"%s\", \"pre_discount_amount\": %.2f, "
         "\"discount\": %.2f}, \"out\": {\"federal_label\": \"%s\", "
         "\"provincial_label\": \"%s\", \"federal_pct\": %.3f, \"provincial_pct\": %.3f, "
         "\"federal\": %.2f, \"provincial\": %.2f}",
         province, amount, discount, rates.federal_label, rates.provincial_label,
         rates.federal_pct, rates.provincial_pct, tr_federal_tax(amount, &rates),
         tr_provincial_tax(amount, discount, &rates));
  close_vector();
}

static void loyalty_vector(const char *id, double amount, double pct) {
  open_vector("loyalty_discount", id);
  printf(", \"in\": {\"amount\": %.2f, \"loyalty_pct\": %.3f}, \"out\": {\"amount\": %.2f}",
         amount, pct, tr_loyalty_discount(amount, pct));
  close_vector();
}

/* The order the loyalty credit and the tax are applied in, expressed with a
   single tax rate so an estate without the province split can be held to it:
   tax sits on the pre discount subtotal, the credit comes off the subtotal. */
static void tail_vector(const char *id, double subtotal, double tax_pct, double loyalty_pct) {
  TrTaxRates rates;
  double tax;
  double loyalty;
  memset(&rates, 0, sizeof(rates));
  rates.federal_pct = tax_pct;
  tax = tr_federal_tax(subtotal, &rates);
  loyalty = tr_loyalty_discount(subtotal, loyalty_pct);
  open_vector("invoice_tail", id);
  printf(", \"in\": {\"subtotal\": %.2f, \"tax_pct\": %.3f, \"loyalty_pct\": %.3f}, "
         "\"out\": {\"tax\": %.2f, \"loyalty\": %.2f, \"total\": %.2f}",
         subtotal, tax_pct, loyalty_pct, tax, loyalty,
         tr_money(subtotal - loyalty + tax));
  close_vector();
}

static void invoice_vector(const char *id, const TrAccount *a, const TrUsage *u) {
  TrInvoice inv;
  tr_compute_invoice(a, u, &inv);
  open_vector("invoice", id);
  printf(", \"in\": {\"province\": \"%s\", \"plan_fee\": %.2f, \"included_gb\": %ld, "
         "\"prev_plan_fee\": %.2f, \"plan_chg_day\": %d, \"line_cnt\": %d, "
         "\"promo_amt\": %.2f, \"promo_dt\": \"%s\", \"susp_start\": %d, \"susp_end\": %d, "
         "\"prior_bal\": %.2f, \"prior_due\": \"%s\", \"loyalty_pct\": %.3f, "
         "\"period\": \"%s\", \"usage_mb\": %ld}",
         a->province, a->plan_fee, a->included_gb, a->prev_plan_fee, a->plan_chg_day,
         a->line_cnt, a->promo_amt, a->promo_dt, a->susp_start, a->susp_end, a->prior_bal,
         a->prior_due, a->loyalty_pct, u->period, u->usage_mb);
  printf(", \"out\": {\"usage_gb_rated\": %ld, \"overage_gb\": %ld, \"plan_charge\": %.2f, "
         "\"line_discount\": %.2f, \"recurring\": %.2f, \"overage_charges\": %.2f, "
         "\"suspension_credit\": %.2f, \"promo_credit\": %.2f, \"late_fee\": %.2f, "
         "\"subtotal\": %.2f, \"loyalty_discount\": %.2f, \"federal_label\": \"%s\", "
         "\"federal_tax\": %.2f, \"provincial_label\": \"%s\", \"provincial_tax\": %.2f, "
         "\"total\": %.2f}",
         inv.usage_gb_rated, inv.overage_gb, inv.plan_charge, inv.line_discount,
         inv.recurring, inv.overage_charges, inv.suspension_credit, inv.promo_credit,
         inv.late_fee, inv.subtotal, inv.loyalty_discount, inv.federal_label,
         inv.federal_tax, inv.provincial_label, inv.provincial_tax, inv.total);
  close_vector();
}

static TrAccount account(const char *province, double plan_fee, long included_gb,
                         double prev_plan_fee, int plan_chg_day, int line_cnt,
                         double promo_amt, const char *promo_dt, int susp_start,
                         int susp_end, double prior_bal, const char *prior_due,
                         double loyalty_pct) {
  TrAccount a;
  memset(&a, 0, sizeof(a));
  strncpy(a.province, province, TR_PROVINCE_LEN - 1);
  a.plan_fee = plan_fee;
  a.included_gb = included_gb;
  a.prev_plan_fee = prev_plan_fee;
  a.plan_chg_day = plan_chg_day;
  a.line_cnt = line_cnt;
  a.promo_amt = promo_amt;
  strncpy(a.promo_dt, promo_dt, TR_DATE_LEN - 1);
  a.susp_start = susp_start;
  a.susp_end = susp_end;
  a.prior_bal = prior_bal;
  strncpy(a.prior_due, prior_due, TR_DATE_LEN - 1);
  a.loyalty_pct = loyalty_pct;
  return a;
}

static TrUsage usage(const char *period, long usage_mb) {
  TrUsage u;
  memset(&u, 0, sizeof(u));
  strncpy(u.period, period, TR_PERIOD_LEN - 1);
  u.usage_mb = usage_mb;
  return u;
}

int main(void) {
  TrAccount a;
  TrUsage u;

  printf("{\n");
  printf("  \"rules_version\": \"%s\",\n", TR_RULES_VERSION);
  printf("  \"source\": \"meridian-telco billing/rules/telco_rules.c\",\n");
  printf("  \"vectors\": [\n");

  money_vector("half-cent-rounds-up", 1.005);
  money_vector("third-of-a-cent-rounds-down", 10.004);
  money_vector("already-cents", 640.00);
  money_vector("long-tail", 128.33333333);

  gb_vector("exact-gigabyte", 1024);
  gb_vector("partial-gigabyte-rounds-up", 1025);
  gb_vector("one-megabyte-is-a-gigabyte", 1);
  gb_vector("zero", 0);

  overage_vector("inside-allowance", 1024L * 1000L, 1000);
  overage_vector("one-megabyte-over-is-a-whole-gigabyte", 1024L * 1000L + 1L, 1000);
  overage_vector("well-over", 7472103L, 5000);
  overage_vector("no-allowance", 4096, 0);

  proration_vector("no-change-is-the-plan-fee", 640.00, 0.00, 0, "2026-07");
  proration_vector("half-and-half", 300.00, 600.00, 16, "2026-07");
  proration_vector("thirty-one-day-month-still-thirty", 300.00, 600.00, 16, "2026-01");
  proration_vector("february-still-thirty", 300.00, 600.00, 16, "2026-02");
  proration_vector("change-on-day-one", 310.00, 620.00, 1, "2026-07");
  proration_vector("change-past-the-thirty-day-month", 310.00, 620.00, 31, "2026-07");

  lines_vector("two-lines", 1000.00, 2);
  lines_vector("three-lines", 1000.00, 3);
  lines_vector("nine-lines", 1000.00, 9);
  lines_vector("ten-lines", 1000.00, 10);
  lines_vector("fourteen-lines-odd-charge", 2333.33, 14);

  promo_vector("issued-in-cycle", 120.00, "2026-07-04", "2026-07");
  promo_vector("issued-late-last-cycle", 120.00, "2026-06-24", "2026-07");
  promo_vector("issued-two-cycles-back", 120.00, "2026-05-20", "2026-07");
  promo_vector("no-issue-date", 120.00, "", "2026-07");
  promo_vector("zero-amount", 0.00, "2026-07-04", "2026-07");

  suspension_vector("five-days-suspended", 620.00, 10, 14, "2026-07");
  suspension_vector("whole-month-suspended", 620.00, 1, 31, "2026-07");
  suspension_vector("not-suspended", 620.00, 0, 0, "2026-07");

  latefee_vector("inside-grace", 500.00, "2026-06-30", "2026-07");
  latefee_vector("exactly-ten-days", 500.00, "2026-06-21", "2026-07");
  latefee_vector("eleven-days", 500.00, "2026-06-20", "2026-07");
  latefee_vector("past-grace", 500.00, "2026-06-01", "2026-07");
  latefee_vector("no-due-date", 500.00, "", "2026-07");
  latefee_vector("no-balance", 0.00, "2026-06-01", "2026-07");

  tax_vector("bc-with-discount", "BC", 100.00, 10.00);
  tax_vector("bc-no-discount", "BC", 100.00, 0.00);
  tax_vector("alberta", "AB", 100.00, 10.00);
  tax_vector("ontario-harmonized", "ON", 100.00, 10.00);
  tax_vector("quebec", "QC", 1000.00, 100.00);
  tax_vector("discount-larger-than-charge", "BC", 50.00, 80.00);
  tax_vector("unknown-province", "ZZ", 100.00, 10.00);

  loyalty_vector("four-percent", 703.13, 4.0);
  loyalty_vector("five-percent", 2400.00, 5.0);
  loyalty_vector("none", 2400.00, 0.0);

  tail_vector("harmonized-thirteen", 703.13, 13.0, 4.0);
  tail_vector("federal-only", 2400.00, 5.0, 5.0);
  tail_vector("no-loyalty", 1250.00, 5.0, 0.0);

  a = account("AB", 2400.00, 2500, 1150.00, 4, 14, 75.00, "2026-07-18", 0, 0, 0.00, "", 5.0);
  u = usage("2026-07", 3145728);
  invoice_vector("alberta-mid-cycle-upgrade", &a, &u);

  a = account("BC", 4200.00, 5000, 2400.00, 4, 14, 350.00, "2026-06-24", 0, 0, 700.90,
              "2026-06-30", 4.0);
  u = usage("2026-07", 7472103);
  invoice_vector("bc-overage-and-dead-promo", &a, &u);

  a = account("ON", 650.00, 500, 0.00, 0, 2, 0.00, "", 10, 14, 1200.00, "2026-05-15", 4.0);
  u = usage("2026-07", 512000);
  invoice_vector("ontario-suspension-and-late-fee", &a, &u);

  a = account("QC", 310.00, 250, 0.00, 0, 11, 500.00, "2026-07-02", 0, 0, 0.00, "", 7.5);
  u = usage("2026-07", 262144);
  invoice_vector("quebec-promo-larger-than-charges", &a, &u);

  printf("\n  ]\n}\n");
  return 0;
}
