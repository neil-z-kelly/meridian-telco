/* Emits the shared conformance vectors on stdout.
 *
 * Every expected value in the file is produced by calling telco_rules.c, never
 * by hand, so the vectors cannot state a rule the library does not implement.
 * Consumers that link or dlopen the library do not need the vectors; the java
 * report module, which does neither, is held to them.
 *
 *   make vectors    # regenerates billing/rules/conformance/vectors.json
 */
#include <stdio.h>

#include "telco_rules.h"

static void rate_case(long usage_mb, long included_gb, int last) {
  printf("      {\"usage_mb\": %ld, \"included_gb\": %ld, \"overage_gb\": %ld, \"amount\": %.2f}%s\n",
         usage_mb, included_gb, tr_overage_gb(usage_mb, included_gb),
         tr_money(tr_rate_overage(usage_mb, included_gb)), last ? "" : ",");
}

static void prorate_case(double fee, double prev_fee, int change_day, int last) {
  printf("      {\"monthly_fee\": %.2f, \"previous_monthly_fee\": %.2f, \"change_day\": %d, "
         "\"amount\": %.2f}%s\n",
         fee, prev_fee, change_day, tr_prorated_plan_charge(fee, prev_fee, change_day),
         last ? "" : ",");
}

static void lines_case(double recurring, int line_count, int last) {
  printf("      {\"recurring_charge\": %.2f, \"line_count\": %d, \"pct\": %.2f, \"amount\": %.2f}%s\n",
         recurring, line_count, tr_multi_line_pct(line_count),
         tr_multi_line_discount(recurring, line_count), last ? "" : ",");
}

static void promo_case(double amount, const char *issued_on, const char *period, int last) {
  printf("      {\"amount\": %.2f, \"issued_on\": \"%s\", \"period\": \"%s\", \"live\": %s, "
         "\"credit\": %.2f}%s\n",
         amount, issued_on, period, tr_promo_is_live(issued_on, period) ? "true" : "false",
         tr_promo_credit(amount, issued_on, period), last ? "" : ",");
}

static void suspend_case(double fee, int start_day, int end_day, int last) {
  printf("      {\"monthly_fee\": %.2f, \"start_day\": %d, \"end_day\": %d, \"days\": %d, "
         "\"credit\": %.2f}%s\n",
         fee, start_day, end_day, tr_suspended_days(start_day, end_day),
         tr_suspension_credit(fee, start_day, end_day), last ? "" : ",");
}

static void latefee_case(double balance, const char *due_date, const char *period, int last) {
  printf("      {\"prior_balance\": %.2f, \"due_date\": \"%s\", \"period\": \"%s\", "
         "\"days_past_due\": %ld, \"fee\": %.2f}%s\n",
         balance, due_date, period, tr_days_past_due(due_date, period),
         tr_late_fee(balance, due_date, period), last ? "" : ",");
}

static void tax_case(const char *province, double subtotal, double discount, int last) {
  tr_tax_rates rates;
  tr_rates_for_province(province, &rates);
  printf("      {\"province\": \"%s\", \"subtotal\": %.2f, \"loyalty_discount\": %.2f, "
         "\"federal_label\": \"%s\", \"federal_pct\": %g, \"federal_tax\": %.2f, "
         "\"provincial_label\": \"%s\", \"provincial_pct\": %g, \"provincial_tax\": %.2f}%s\n",
         province, subtotal, discount, rates.federal_label, rates.federal_pct,
         tr_federal_tax(subtotal, &rates), rates.provincial_label, rates.provincial_pct,
         tr_provincial_tax(subtotal, discount, &rates), last ? "" : ",");
}

static void loyalty_case(double subtotal, double pct, int last) {
  printf("      {\"subtotal\": %.2f, \"loyalty_pct\": %.2f, \"discount\": %.2f}%s\n", subtotal, pct,
         tr_loyalty_discount(subtotal, pct), last ? "" : ",");
}

static void round_case(double amount, int last) {
  printf("      {\"amount\": %.6f, \"rounded\": %.2f}%s\n", amount, tr_money(amount),
         last ? "" : ",");
}

static void invoice_case(const char *name, tr_account a, long usage_mb, const char *period,
                         int last) {
  tr_invoice inv;
  tr_compute_invoice(&a, usage_mb, period, &inv);
  printf("      {\n");
  printf("        \"name\": \"%s\",\n", name);
  printf("        \"account\": {\"province\": \"%s\", \"plan_fee\": %.2f, \"included_gb\": %ld, "
         "\"prev_plan_fee\": %.2f, \"plan_change_day\": %d, \"line_count\": %d, "
         "\"promo_amount\": %.2f, \"promo_issued_on\": \"%s\", \"suspension_start_day\": %d, "
         "\"suspension_end_day\": %d, \"prior_balance\": %.2f, \"prior_due_date\": \"%s\", "
         "\"loyalty_pct\": %.2f},\n",
         a.province, a.plan_fee, a.included_gb, a.prev_plan_fee, a.plan_change_day, a.line_count,
         a.promo_amount, a.promo_issued_on, a.suspension_start_day, a.suspension_end_day,
         a.prior_balance, a.prior_due_date, a.loyalty_pct);
  printf("        \"usage_mb\": %ld,\n", usage_mb);
  printf("        \"period\": \"%s\",\n", period);
  printf("        \"expect\": {\"usage_gb_rated\": %ld, \"overage_gb\": %ld, \"plan_charge\": %.2f, "
         "\"line_discount\": %.2f, \"recurring\": %.2f, \"overage_charges\": %.2f, "
         "\"suspension_credit\": %.2f, \"promo_credit\": %.2f, \"late_fee\": %.2f, "
         "\"subtotal\": %.2f, \"loyalty_discount\": %.2f, \"federal_label\": \"%s\", "
         "\"federal_tax\": %.2f, \"provincial_label\": \"%s\", \"provincial_tax\": %.2f, "
         "\"total\": %.2f}\n",
         inv.usage_gb_rated, inv.overage_gb, inv.plan_charge, inv.line_discount, inv.recurring,
         inv.overage_charges, inv.suspension_credit, inv.promo_credit, inv.late_fee, inv.subtotal,
         inv.loyalty_discount, inv.federal_label, inv.federal_tax, inv.provincial_label,
         inv.provincial_tax, inv.total);
  printf("      }%s\n", last ? "" : ",");
}

static tr_account account(const char *province, double plan_fee, long included_gb,
                          double prev_plan_fee, int plan_change_day, int line_count,
                          double promo_amount, const char *promo_issued_on, int susp_start,
                          int susp_end, double prior_balance, const char *prior_due_date,
                          double loyalty_pct) {
  tr_account a;
  int i;
  for (i = 0; i < TR_LABEL_LEN; i++) a.province[i] = '\0';
  for (i = 0; province[i] && i < TR_LABEL_LEN - 1; i++) a.province[i] = province[i];
  a.plan_fee = plan_fee;
  a.included_gb = included_gb;
  a.prev_plan_fee = prev_plan_fee;
  a.plan_change_day = plan_change_day;
  a.line_count = line_count;
  a.promo_amount = promo_amount;
  for (i = 0; i < TR_DATE_LEN; i++) a.promo_issued_on[i] = '\0';
  for (i = 0; promo_issued_on[i] && i < TR_DATE_LEN - 1; i++)
    a.promo_issued_on[i] = promo_issued_on[i];
  a.suspension_start_day = susp_start;
  a.suspension_end_day = susp_end;
  a.prior_balance = prior_balance;
  for (i = 0; i < TR_DATE_LEN; i++) a.prior_due_date[i] = '\0';
  for (i = 0; prior_due_date[i] && i < TR_DATE_LEN - 1; i++)
    a.prior_due_date[i] = prior_due_date[i];
  a.loyalty_pct = loyalty_pct;
  return a;
}

int main(void) {
  printf("{\n");
  printf("  \"version\": \"%s\",\n", tr_rules_version());
  printf("  \"source\": \"meridian-telco billing/rules/telco_rules.c\",\n");
  printf("  \"generated_by\": \"bin/rules-vectors (make vectors)\",\n");
  printf("  \"rules\": {\n");

  printf("    \"R-RATE\": [\n");
  rate_case(0, 2000, 0);
  rate_case(1000, 2000, 0);
  rate_case(2000 * 1024, 2000, 0);
  rate_case(2000 * 1024 + 1, 2000, 0);
  rate_case(2000 * 1024 + 1000, 2000, 0);
  rate_case(3 * 1024 + 1, 2, 0);
  rate_case(3000000, 2000, 1);
  printf("    ],\n");

  printf("    \"R-PRORATE\": [\n");
  prorate_case(640.0, 0.0, 0, 0);
  prorate_case(300.0, 600.0, 16, 0);
  prorate_case(310.0, 620.0, 31, 0);
  prorate_case(280.0, 560.0, 15, 0);
  prorate_case(4200.0, 2400.0, 4, 1);
  printf("    ],\n");

  printf("    \"R-LINES\": [\n");
  lines_case(1000.0, 1, 0);
  lines_case(1000.0, 3, 0);
  lines_case(1000.0, 9, 0);
  lines_case(1000.0, 12, 0);
  lines_case(4200.0, 14, 1);
  printf("    ],\n");

  printf("    \"R-PROMO\": [\n");
  promo_case(120.0, "2026-07-04", "2026-07", 0);
  promo_case(120.0, "2026-06-24", "2026-07", 0);
  promo_case(120.0, "2026-05-20", "2026-07", 0);
  promo_case(0.0, "2026-07-04", "2026-07", 1);
  printf("    ],\n");

  printf("    \"R-SUSPEND\": [\n");
  suspend_case(620.0, 10, 14, 0);
  suspend_case(620.0, 0, 0, 0);
  suspend_case(620.0, 1, 31, 1);
  printf("    ],\n");

  printf("    \"R-LATEFEE\": [\n");
  latefee_case(500.0, "2026-06-30", "2026-07", 0);
  latefee_case(500.0, "2026-06-01", "2026-07", 0);
  latefee_case(700.90, "2026-06-15", "2026-07", 0);
  latefee_case(0.0, "2026-06-01", "2026-07", 1);
  printf("    ],\n");

  printf("    \"R-TAXBASE\": [\n");
  tax_case("BC", 100.0, 10.0, 0);
  tax_case("AB", 100.0, 10.0, 0);
  tax_case("ON", 100.0, 10.0, 0);
  tax_case("QC", 1000.0, 100.0, 0);
  tax_case("ZZ", 100.0, 10.0, 1);
  printf("    ],\n");

  printf("    \"R-LOYALTY\": [\n");
  loyalty_case(1000.0, 8.0, 0);
  loyalty_case(1000.0, 0.0, 0);
  loyalty_case(4321.55, 4.0, 1);
  printf("    ],\n");

  printf("    \"R-ROUND\": [\n");
  round_case(1.005, 0);
  round_case(1.004999, 0);
  round_case(2.675, 0);
  round_case(1234.5649, 1);
  printf("    ]\n");
  printf("  },\n");

  printf("  \"invoices\": [\n");
  invoice_case("bc enterprise, mid cycle upgrade, promo from last cycle",
               account("BC", 4200.0, 5000, 2400.0, 4, 14, 350.0, "2026-06-24", 0, 0, 700.90,
                       "2026-06-30", 4.0),
               5300000, "2026-07", 0);
  invoice_case("on business, suspended fortnight, promo in cycle",
               account("ON", 650.0, 500, 0.0, 0, 2, 120.0, "2026-07-04", 10, 24, 0.0, "", 0.0),
               620000, "2026-07", 0);
  invoice_case("qc metro burst, late balance, loyalty",
               account("QC", 310.0, 250, 0.0, 0, 4, 0.0, "", 0, 0, 480.25, "2026-06-01", 2.5),
               300000, "2026-07", 0);
  invoice_case("ab flat rate, no usage over allowance",
               account("AB", 2400.0, 3000, 0.0, 0, 11, 0.0, "", 0, 0, 0.0, "", 8.0), 1024, "2026-07",
               1);
  printf("  ]\n");
  printf("}\n");
  return 0;
}
