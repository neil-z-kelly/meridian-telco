#include "telco_rules.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Shared billing rules, legacy behaviour. See telco_rules.h and RULES.md. */

typedef struct {
  int y;
  int m;
  int d;
  int ok;
} TrDate;

static TrDate parse_date(const char *s) {
  TrDate out;
  char buf[5];
  out.y = 0;
  out.m = 0;
  out.d = 0;
  out.ok = 0;
  if (s == NULL || strlen(s) < 10) return out;
  memcpy(buf, s, 4);
  buf[4] = '\0';
  out.y = atoi(buf);
  memcpy(buf, s + 5, 2);
  buf[2] = '\0';
  out.m = atoi(buf);
  memcpy(buf, s + 8, 2);
  buf[2] = '\0';
  out.d = atoi(buf);
  out.ok = out.y > 0 && out.m > 0 && out.d > 0;
  return out;
}

static TrDate period_start(const char *period) {
  TrDate out;
  char buf[5];
  out.y = 0;
  out.m = 0;
  out.d = 1;
  out.ok = 0;
  if (period == NULL || strlen(period) < 7) return out;
  memcpy(buf, period, 4);
  buf[4] = '\0';
  out.y = atoi(buf);
  memcpy(buf, period + 5, 2);
  buf[2] = '\0';
  out.m = atoi(buf);
  out.ok = out.y > 0 && out.m > 0;
  return out;
}

/* days since 1970-01-01, howard hinnant's days_from_civil. */
static long serial_day(TrDate dt) {
  int y = dt.y - (dt.m <= 2);
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (unsigned)((153 * (dt.m + (dt.m > 2 ? -3 : 9)) + 2) / 5 + dt.d - 1);
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}

double tr_money(double amount) {
  return floor(amount * 100.0 + 0.5) / 100.0;
}

long tr_usage_gb_rounded(long usage_mb) {
  long gb = usage_mb / TR_MB_PER_GB;
  if (usage_mb % TR_MB_PER_GB) gb++;
  return gb;
}

long tr_overage_gb(long usage_mb, long included_gb) {
  long gb = tr_usage_gb_rounded(usage_mb);
  if (gb <= included_gb) return 0;
  return gb - included_gb;
}

double tr_rate_overage(long usage_mb, long included_gb) {
  return tr_money((double)tr_overage_gb(usage_mb, included_gb) * TR_OVERAGE_RATE_PER_GB);
}

double tr_daily_rate(double monthly_fee, const char *period) {
  (void)period;
  return monthly_fee / (double)TR_BILLING_MONTH_DAYS;
}

double tr_prorated_plan_charge(double monthly_fee, double prev_monthly_fee,
                               int change_day, const char *period) {
  int days_on_old;
  int days_on_new;
  double old_part;
  double new_part;
  if (change_day <= 0) return tr_money(monthly_fee);
  days_on_old = change_day - 1;
  if (days_on_old < 0) days_on_old = 0;
  if (days_on_old > TR_BILLING_MONTH_DAYS) days_on_old = TR_BILLING_MONTH_DAYS;
  days_on_new = TR_BILLING_MONTH_DAYS - days_on_old;
  old_part = tr_money(tr_daily_rate(prev_monthly_fee, period) * (double)days_on_old);
  new_part = tr_money(tr_daily_rate(monthly_fee, period) * (double)days_on_new);
  return tr_money(old_part + new_part);
}

double tr_multi_line_pct(int line_count) {
  if (line_count >= 10) return 10.0;
  if (line_count >= 3) return 5.0;
  return 0.0;
}

double tr_multi_line_discount(double recurring_charge, int line_count) {
  return tr_money(recurring_charge * tr_multi_line_pct(line_count) / 100.0);
}

int tr_promo_is_live(const char *issued_on, const char *period) {
  TrDate issued = parse_date(issued_on);
  TrDate cycle = period_start(period);
  if (!issued.ok || !cycle.ok) return 0;
  return issued.y == cycle.y && issued.m == cycle.m;
}

double tr_promo_credit(double amount, const char *issued_on, const char *period) {
  if (amount <= 0.0) return 0.0;
  if (!tr_promo_is_live(issued_on, period)) return 0.0;
  return tr_money(amount);
}

int tr_suspended_days(int start_day, int end_day) {
  if (start_day <= 0 || end_day < start_day) return 0;
  return end_day - start_day + 1;
}

double tr_suspension_credit(double monthly_fee, int start_day, int end_day,
                            const char *period) {
  (void)monthly_fee;
  (void)start_day;
  (void)end_day;
  (void)period;
  return 0.0;
}

long tr_days_past_due(const char *due_date, const char *period) {
  TrDate due = parse_date(due_date);
  TrDate cycle = period_start(period);
  long days;
  if (!due.ok || !cycle.ok) return 0;
  days = serial_day(cycle) - serial_day(due);
  return days > 0 ? days : 0;
}

double tr_late_fee(double prior_balance, const char *due_date, const char *period) {
  if (prior_balance <= 0.0) return 0.0;
  if (tr_days_past_due(due_date, period) <= TR_LATE_FEE_GRACE_DAYS) return 0.0;
  return tr_money(prior_balance * TR_LATE_FEE_PCT / 100.0);
}

static void set_rates(TrTaxRates *out, double fed, double prov, const char *fed_label,
                      const char *prov_label) {
  out->federal_pct = fed;
  out->provincial_pct = prov;
  strncpy(out->federal_label, fed_label, TR_LABEL_LEN - 1);
  out->federal_label[TR_LABEL_LEN - 1] = '\0';
  strncpy(out->provincial_label, prov_label, TR_LABEL_LEN - 1);
  out->provincial_label[TR_LABEL_LEN - 1] = '\0';
}

void tr_rates_for_province(const char *province, TrTaxRates *out) {
  char code[TR_PROVINCE_LEN];
  size_t i;
  if (out == NULL) return;
  code[0] = '\0';
  if (province != NULL) {
    strncpy(code, province, TR_PROVINCE_LEN - 1);
    code[TR_PROVINCE_LEN - 1] = '\0';
    for (i = 0; i < strlen(code); i++) {
      if (code[i] >= 'a' && code[i] <= 'z') code[i] = (char)(code[i] - 'a' + 'A');
    }
  }
  if (strcmp(code, "BC") == 0) {
    set_rates(out, 5.0, 7.0, "GST", "PST");
  } else if (strcmp(code, "ON") == 0) {
    set_rates(out, 13.0, 0.0, "HST", "");
  } else if (strcmp(code, "QC") == 0) {
    set_rates(out, 5.0, 9.975, "GST", "QST");
  } else {
    /* AB and anything unrecognised: federal only. */
    set_rates(out, 5.0, 0.0, "GST", "");
  }
}

double tr_federal_tax(double pre_discount_amount, const TrTaxRates *rates) {
  if (rates == NULL) return 0.0;
  return tr_money(pre_discount_amount * rates->federal_pct / 100.0);
}

double tr_provincial_tax(double pre_discount_amount, double discount,
                         const TrTaxRates *rates) {
  double base;
  if (rates == NULL || rates->provincial_pct <= 0.0) return 0.0;
  base = pre_discount_amount - discount;
  if (base < 0.0) base = 0.0;
  return tr_money(base * rates->provincial_pct / 100.0);
}

double tr_loyalty_discount(double amount, double loyalty_pct) {
  return tr_money(amount * loyalty_pct / 100.0);
}

void tr_compute_invoice(const TrAccount *a, const TrUsage *u, TrInvoice *out) {
  TrTaxRates rates;
  double subtotal;
  if (a == NULL || u == NULL || out == NULL) return;
  memset(out, 0, sizeof(*out));

  out->usage_gb_rated = tr_usage_gb_rounded(u->usage_mb);
  out->overage_gb = tr_overage_gb(u->usage_mb, a->included_gb);
  out->plan_charge = tr_prorated_plan_charge(a->plan_fee, a->prev_plan_fee,
                                             a->plan_chg_day, u->period);
  out->line_discount = tr_multi_line_discount(out->plan_charge, a->line_cnt);
  out->recurring = tr_money(out->plan_charge - out->line_discount);
  out->overage_charges = tr_rate_overage(u->usage_mb, a->included_gb);
  out->suspension_credit =
      tr_suspension_credit(a->plan_fee, a->susp_start, a->susp_end, u->period);
  out->promo_credit = tr_promo_credit(a->promo_amt, a->promo_dt, u->period);
  out->late_fee = tr_late_fee(a->prior_bal, a->prior_due, u->period);

  subtotal = out->recurring + out->overage_charges + out->late_fee
             - out->suspension_credit - out->promo_credit;
  if (subtotal < 0.0) subtotal = 0.0;
  out->subtotal = tr_money(subtotal);

  tr_rates_for_province(a->province, &rates);
  out->loyalty_discount = tr_loyalty_discount(out->subtotal, a->loyalty_pct);
  out->federal_tax = tr_federal_tax(out->subtotal, &rates);
  out->provincial_tax = tr_provincial_tax(out->subtotal, out->loyalty_discount, &rates);
  strncpy(out->federal_label, rates.federal_label, TR_LABEL_LEN - 1);
  strncpy(out->provincial_label, rates.provincial_label, TR_LABEL_LEN - 1);
  out->total = tr_money(out->subtotal - out->loyalty_discount + out->federal_tax
                        + out->provincial_tax);
}
