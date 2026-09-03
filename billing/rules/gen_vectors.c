#include "billing_rules.h"

#include <stdio.h>
#include <string.h>

/* Writes the conformance vectors every engine is held to.

   Two files, both generated from billing_rules.c and never edited by hand:

     conformance_vectors.csv  one row per rule call, rule,a1,a2,a3,expected
     invoice_vectors.csv      whole invoices, account fields then invoice lines

   Run `make vectors` to regenerate and `make check` to fail when the checked
   in files no longer match what the rules produce. */

static void rule_row(FILE *f, const char *rule, const char *a1, const char *a2,
                     const char *a3, double expected) {
  fprintf(f, "%s,%s,%s,%s,%.6f\n", rule, a1, a2, a3, expected);
}

static const char *num(char *buf, size_t size, double v) {
  snprintf(buf, size, "%.6f", v);
  return buf;
}

static const char *inum(char *buf, size_t size, long v) {
  snprintf(buf, size, "%ld", v);
  return buf;
}

static void tax_rates_of(double federal_pct, double provincial_pct, br_tax_rates *out) {
  memset(out, 0, sizeof(*out));
  out->federal_pct = federal_pct;
  out->provincial_pct = provincial_pct;
}

static void write_rule_vectors(FILE *f) {
  static const double money_in[] = {0.0, 1.005, 2.344, 2.345, 1234.5649, 99.999, 0.004};
  static const long usage_in[] = {0, 1, 1023, 1024, 1025, 5813259, 7472103};
  static const long included_in[] = {0, 500, 2500, 5000};
  static const double fee_in[] = {310.0, 640.0, 2400.0, 4200.0};
  static const int chg_day_in[] = {0, 1, 4, 16, 30, 31};
  static const int line_in[] = {1, 2, 3, 9, 10, 14};
  static const char *province_in[] = {"BC", "AB", "ON", "QC", "SK", ""};
  static const char *date_in[] = {"2026-05-20", "2026-06-24", "2026-06-30", "2026-07-04", ""};
  static const double pct_in[] = {0.0, 2.5, 4.0, 5.0, 8.0};
  char b1[32];
  char b2[32];
  char b3[32];
  size_t i;
  size_t j;
  br_tax_rates rates;

  fprintf(f, "rule,arg1,arg2,arg3,expected\n");

  for (i = 0; i < sizeof(money_in) / sizeof(money_in[0]); i++) {
    rule_row(f, "money", num(b1, sizeof(b1), money_in[i]), "", "", br_money(money_in[i]));
  }

  for (i = 0; i < sizeof(usage_in) / sizeof(usage_in[0]); i++) {
    rule_row(f, "usage_gb_rounded", inum(b1, sizeof(b1), usage_in[i]), "", "",
             (double)br_usage_gb_rounded(usage_in[i]));
    for (j = 0; j < sizeof(included_in) / sizeof(included_in[0]); j++) {
      rule_row(f, "overage_gb", inum(b1, sizeof(b1), usage_in[i]),
               inum(b2, sizeof(b2), included_in[j]), "",
               (double)br_overage_gb(usage_in[i], included_in[j]));
      rule_row(f, "rate_overage", inum(b1, sizeof(b1), usage_in[i]),
               inum(b2, sizeof(b2), included_in[j]), "",
               br_rate_overage(usage_in[i], included_in[j]));
    }
  }

  for (i = 0; i < sizeof(fee_in) / sizeof(fee_in[0]); i++) {
    rule_row(f, "daily_rate", num(b1, sizeof(b1), fee_in[i]), "", "", br_daily_rate(fee_in[i]));
    for (j = 0; j < sizeof(chg_day_in) / sizeof(chg_day_in[0]); j++) {
      rule_row(f, "prorated_plan_charge", num(b1, sizeof(b1), fee_in[i]),
               num(b2, sizeof(b2), fee_in[i] / 2.0), inum(b3, sizeof(b3), chg_day_in[j]),
               br_prorated_plan_charge(fee_in[i], fee_in[i] / 2.0, chg_day_in[j]));
    }
    rule_row(f, "suspension_credit", num(b1, sizeof(b1), fee_in[i]), "10", "14",
             br_suspension_credit(fee_in[i], 10, 14));
  }

  rule_row(f, "suspended_days", "0", "0", "", (double)br_suspended_days(0, 0));
  rule_row(f, "suspended_days", "10", "14", "", (double)br_suspended_days(10, 14));
  rule_row(f, "suspended_days", "14", "10", "", (double)br_suspended_days(14, 10));

  for (i = 0; i < sizeof(line_in) / sizeof(line_in[0]); i++) {
    rule_row(f, "multi_line_pct", inum(b1, sizeof(b1), line_in[i]), "", "",
             br_multi_line_pct(line_in[i]));
    rule_row(f, "multi_line_discount", "1000.000000", inum(b1, sizeof(b1), line_in[i]), "",
             br_multi_line_discount(1000.0, line_in[i]));
  }

  for (i = 0; i < sizeof(date_in) / sizeof(date_in[0]); i++) {
    rule_row(f, "promo_is_live", date_in[i], "2026-07", "",
             (double)br_promo_is_live(date_in[i], "2026-07"));
    rule_row(f, "promo_credit", "120.000000", date_in[i], "2026-07",
             br_promo_credit(120.0, date_in[i], "2026-07"));
    rule_row(f, "days_past_due", date_in[i], "2026-07", "",
             (double)br_days_past_due(date_in[i], "2026-07"));
    rule_row(f, "late_fee", "500.000000", date_in[i], "2026-07",
             br_late_fee(500.0, date_in[i], "2026-07"));
  }

  for (i = 0; i < sizeof(province_in) / sizeof(province_in[0]); i++) {
    br_rates_for_province(province_in[i], &rates);
    rule_row(f, "province_federal_pct", province_in[i], "", "", rates.federal_pct);
    rule_row(f, "province_provincial_pct", province_in[i], "", "", rates.provincial_pct);
    fprintf(f, "province_labels,%s,%s,%s,0.000000\n", province_in[i], rates.federal_label,
            rates.provincial_label);
  }

  /* tax and loyalty are held to the rule, not to a province: the rate is an
     input so an engine with its own rate table is still pinned to the base
     each component is charged on. */
  for (i = 0; i < sizeof(pct_in) / sizeof(pct_in[0]); i++) {
    tax_rates_of(pct_in[i], pct_in[i], &rates);
    rule_row(f, "federal_tax", "1000.000000", num(b1, sizeof(b1), pct_in[i]), "",
             br_federal_tax(1000.0, &rates));
    rule_row(f, "provincial_tax", "1000.000000", "100.000000", num(b1, sizeof(b1), pct_in[i]),
             br_provincial_tax(1000.0, 100.0, &rates));
    rule_row(f, "loyalty_discount", "1000.000000", num(b1, sizeof(b1), pct_in[i]), "",
             br_loyalty_discount(1000.0, pct_in[i]));
  }
}

typedef struct {
  const char *acct_id;
  const char *province;
  double plan_fee;
  long included_gb;
  double prev_plan_fee;
  int plan_chg_day;
  int line_cnt;
  double promo_amt;
  const char *promo_dt;
  int susp_start;
  int susp_end;
  double prior_bal;
  const char *prior_due;
  double loyalty_pct;
  long usage_mb;
  const char *period;
} vector_case;

static void write_invoice_vectors(FILE *f) {
  static const vector_case cases[] = {
    {"VEC-0001", "BC", 4200.0, 5000, 2400.0, 4, 14, 350.0, "2026-06-24", 0, 0, 700.90,
     "2026-06-30", 4.0, 7472103, "2026-07"},
    {"VEC-0002", "AB", 2400.0, 2500, 0.0, 0, 1, 200.0, "2026-07-18", 0, 0, 0.0, "", 0.0,
     3146111, "2026-07"},
    {"VEC-0003", "ON", 650.0, 1000, 1300.0, 16, 4, 0.0, "", 10, 14, 480.0, "2026-06-01", 5.0,
     1240000, "2026-07"},
    {"VEC-0004", "QC", 310.0, 250, 0.0, 0, 12, 90.0, "2026-07-02", 3, 20, 125.5, "2026-06-15",
     8.0, 262144, "2026-07"},
    {"VEC-0005", "QC", 310.0, 250, 620.0, 31, 2, 500.0, "2026-06-30", 0, 0, 0.0, "", 2.5, 0,
     "2026-07"},
    {"VEC-0006", "SK", 650.0, 500, 0.0, 0, 10, 0.0, "", 0, 0, 40.0, "2026-06-28", 0.0, 512001,
     "2026-02"}
  };
  size_t i;

  fprintf(f, "acct_id,province,plan_fee,included_gb,prev_plan_fee,plan_chg_day,line_cnt,"
             "promo_amt,promo_dt,susp_start,susp_end,prior_bal,prior_due,loyalty_pct,"
             "usage_mb,period,usage_gb_rated,overage_gb,plan_charge,line_discount,recurring,"
             "overage_charges,suspension_credit,promo_credit,late_fee,subtotal,loyalty,"
             "federal_tax,federal_label,provincial_tax,provincial_label,total\n");

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    const vector_case *c = &cases[i];
    br_account a;
    br_invoice inv;
    br_account_init(&a);
    br_set_field(a.acct_id, sizeof(a.acct_id), c->acct_id);
    br_set_field(a.province, sizeof(a.province), c->province);
    a.plan_fee = c->plan_fee;
    a.included_gb = c->included_gb;
    a.prev_plan_fee = c->prev_plan_fee;
    a.plan_chg_day = c->plan_chg_day;
    a.line_cnt = c->line_cnt;
    a.promo_amt = c->promo_amt;
    br_set_field(a.promo_dt, sizeof(a.promo_dt), c->promo_dt);
    a.susp_start = c->susp_start;
    a.susp_end = c->susp_end;
    a.prior_bal = c->prior_bal;
    br_set_field(a.prior_due, sizeof(a.prior_due), c->prior_due);
    a.loyalty_pct = c->loyalty_pct;

    br_compute_invoice(&a, c->usage_mb, c->period, &inv);

    fprintf(f,
            "%s,%s,%.2f,%ld,%.2f,%d,%d,%.2f,%s,%d,%d,%.2f,%s,%.2f,%ld,%s,"
            "%ld,%ld,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%.2f,%s,%.2f\n",
            c->acct_id, c->province, c->plan_fee, c->included_gb, c->prev_plan_fee,
            c->plan_chg_day, c->line_cnt, c->promo_amt, c->promo_dt, c->susp_start,
            c->susp_end, c->prior_bal, c->prior_due, c->loyalty_pct, c->usage_mb, c->period,
            inv.usage_gb_rated, inv.overage_gb, inv.plan_charge, inv.line_discount,
            inv.recurring, inv.overage_charges, inv.suspension_credit, inv.promo_credit,
            inv.late_fee, inv.subtotal, inv.loyalty, inv.federal_tax, inv.federal_label,
            inv.provincial_tax, inv.provincial_label, inv.total);
  }
}

int main(int argc, char **argv) {
  const char *rules_path = "billing/rules/conformance_vectors.csv";
  const char *invoice_path = "billing/rules/invoice_vectors.csv";
  FILE *f;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--rules") == 0 && i + 1 < argc) rules_path = argv[++i];
    else if (strcmp(argv[i], "--invoices") == 0 && i + 1 < argc) invoice_path = argv[++i];
  }

  f = fopen(rules_path, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", rules_path); return 1; }
  write_rule_vectors(f);
  fclose(f);

  f = fopen(invoice_path, "w");
  if (!f) { fprintf(stderr, "cannot write %s\n", invoice_path); return 1; }
  write_invoice_vectors(f);
  fclose(f);
  return 0;
}
