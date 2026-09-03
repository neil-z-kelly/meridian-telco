/* telco_rules — see telco_rules.h.
 *
 * Every rule here is the legacy meridian behaviour. Where meridian and vantage
 * disagreed the meridian reading is the one implemented, and the disagreement is
 * recorded in billing/rules/RULES.md. Rules with a finance sign-off carry the
 * original note from the file they came out of.
 */
#include "telco_rules.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- rounding */

double tr_money(double amount) {
  return floor(amount * 100.0 + 0.5) / 100.0;
}

/* ------------------------------------------------------------------ rating */

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
  return (double)tr_overage_gb(usage_mb, included_gb) * TR_OVERAGE_RATE_PER_GB;
}

/* --------------------------------------------------------------- proration */

/* the billing month is 30 days, always, whatever the calendar says. this is the
   convention the register has used since the first cycle and the downstream
   reconciliation reports assume it. RS 2010-02 */

double tr_daily_rate(double monthly_fee) {
  return monthly_fee / (double)TR_BILLING_MONTH_DAYS;
}

double tr_prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  int days_on_old;
  int days_on_new;
  double old_part;
  double new_part;
  if (change_day <= 0) return tr_money(monthly_fee);
  days_on_old = change_day - 1;
  if (days_on_old < 0) days_on_old = 0;
  if (days_on_old > TR_BILLING_MONTH_DAYS) days_on_old = TR_BILLING_MONTH_DAYS;
  days_on_new = TR_BILLING_MONTH_DAYS - days_on_old;
  old_part = tr_money(tr_daily_rate(prev_monthly_fee) * (double)days_on_old);
  new_part = tr_money(tr_daily_rate(monthly_fee) * (double)days_on_new);
  return tr_money(old_part + new_part);
}

/* ------------------------------------------------------------- multi -line */

double tr_multi_line_pct(int line_count) {
  if (line_count >= 10) return 10.0;
  if (line_count >= 3) return 5.0;
  return 0.0;
}

double tr_multi_line_discount(double recurring_charge, int line_count) {
  return tr_money(recurring_charge * tr_multi_line_pct(line_count) / 100.0);
}

/* ------------------------------------------------------------ date helpers */

typedef struct {
  int y;
  int m;
  int d;
  int ok;
} tr_date;

static int tr_atoi_n(const char *s, int n) {
  char buf[8];
  int i;
  if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
  for (i = 0; i < n; i++) buf[i] = s[i];
  buf[n] = '\0';
  return atoi(buf);
}

static tr_date tr_parse_date(const char *s) {
  tr_date out;
  out.y = 0; out.m = 0; out.d = 0; out.ok = 0;
  if (s == NULL || strlen(s) < 10) return out;
  out.y = tr_atoi_n(s, 4);
  out.m = tr_atoi_n(s + 5, 2);
  out.d = tr_atoi_n(s + 8, 2);
  out.ok = out.y > 0 && out.m > 0 && out.d > 0;
  return out;
}

static tr_date tr_period_start(const char *period) {
  tr_date out;
  out.y = 0; out.m = 0; out.d = 1; out.ok = 0;
  if (period == NULL || strlen(period) < 7) return out;
  out.y = tr_atoi_n(period, 4);
  out.m = tr_atoi_n(period + 5, 2);
  out.ok = out.y > 0 && out.m > 0;
  return out;
}

/* days since 1970-01-01, howard hinnant's civil_from_days in reverse. */
static long tr_serial_day(tr_date dt) {
  int y = dt.y;
  long era;
  unsigned yoe;
  unsigned doy;
  unsigned doe;
  y -= dt.m <= 2;
  era = (y >= 0 ? y : y - 399) / 400;
  yoe = (unsigned)(y - era * 400);
  doy = (153 * (dt.m + (dt.m > 2 ? -3 : 9)) + 2) / 5 + dt.d - 1;
  doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}

/* ------------------------------------------------------------------- promo */

/* promotional credits expire at the end of the billing cycle they were issued
   in. a credit issued in june is a june credit, it does not roll into july.
   care wanted 30 days once, finance said no, the cycle is the unit. RS 2012-08 */

int tr_promo_is_live(const char *issued_on, const char *period) {
  tr_date issued = tr_parse_date(issued_on);
  tr_date cycle = tr_period_start(period);
  if (!issued.ok || !cycle.ok) return 0;
  return issued.y == cycle.y && issued.m == cycle.m;
}

double tr_promo_credit(double amount, const char *issued_on, const char *period) {
  if (amount <= 0.0) return 0.0;
  if (!tr_promo_is_live(issued_on, period)) return 0.0;
  return tr_money(amount);
}

/* -------------------------------------------------------------- suspension */

/* a suspended line keeps its number, its provisioning and its place on the
   switch, so the month is billed in full. there is no partial month credit for
   suspension. RS 2013-05 */

int tr_suspended_days(int start_day, int end_day) {
  if (start_day <= 0 || end_day < start_day) return 0;
  return end_day - start_day + 1;
}

double tr_suspension_credit(double monthly_fee, int start_day, int end_day) {
  (void)monthly_fee;
  (void)start_day;
  (void)end_day;
  return 0.0;
}

/* ---------------------------------------------------------------- late fee */

long tr_days_past_due(const char *due_date, const char *period) {
  tr_date due = tr_parse_date(due_date);
  tr_date cycle = tr_period_start(period);
  long days;
  if (!due.ok || !cycle.ok) return 0;
  days = tr_serial_day(cycle) - tr_serial_day(due);
  return days > 0 ? days : 0;
}

double tr_late_fee(double prior_balance, const char *due_date, const char *period) {
  if (prior_balance <= 0.0) return 0.0;
  if (tr_days_past_due(due_date, period) <= TR_LATE_FEE_GRACE_DAYS) return 0.0;
  return tr_money(prior_balance * TR_LATE_FEE_PCT / 100.0);
}

/* --------------------------------------------------------------------- tax */

/* GST (and HST where the province harmonized) is assessed on the charge before
   any loyalty discount. CRA treats the discount as a goodwill credit and not a
   reduction of consideration.

   the provincial component is the one finance ruled on locally: PST and QST are
   assessed on what the customer actually pays, so the loyalty discount comes off
   first. signed off 2010. RS 2010-06 */

static void tr_set_rates(tr_tax_rates *out, double fed, double prov, const char *fed_label,
                         const char *prov_label) {
  out->federal_pct = fed;
  out->provincial_pct = prov;
  strncpy(out->federal_label, fed_label, TR_LABEL_LEN - 1);
  out->federal_label[TR_LABEL_LEN - 1] = '\0';
  strncpy(out->provincial_label, prov_label, TR_LABEL_LEN - 1);
  out->provincial_label[TR_LABEL_LEN - 1] = '\0';
}

void tr_rates_for_province(const char *province, tr_tax_rates *out) {
  const char *p = province == NULL ? "" : province;
  if (strcmp(p, "BC") == 0) { tr_set_rates(out, 5.0, 7.0, "GST", "PST"); return; }
  if (strcmp(p, "AB") == 0) { tr_set_rates(out, 5.0, 0.0, "GST", ""); return; }
  if (strcmp(p, "ON") == 0) { tr_set_rates(out, 13.0, 0.0, "HST", ""); return; }
  if (strcmp(p, "QC") == 0) { tr_set_rates(out, 5.0, 9.975, "GST", "QST"); return; }
  tr_set_rates(out, 5.0, 0.0, "GST", "");
}

double tr_federal_tax(double pre_discount_amount, const tr_tax_rates *rates) {
  return tr_money(pre_discount_amount * rates->federal_pct / 100.0);
}

double tr_provincial_tax(double pre_discount_amount, double discount, const tr_tax_rates *rates) {
  double base;
  if (rates->provincial_pct <= 0.0) return 0.0;
  base = pre_discount_amount - discount;
  if (base < 0.0) base = 0.0;
  return tr_money(base * rates->provincial_pct / 100.0);
}

/* ----------------------------------------------------------------- loyalty */

double tr_loyalty_discount(double amount, double loyalty_pct) {
  return tr_money(amount * loyalty_pct / 100.0);
}

/* ----------------------------------------------------------------- invoice */

void tr_compute_invoice(const tr_account *a, long usage_mb, const char *period, tr_invoice *out) {
  tr_tax_rates rates;
  double subtotal;

  out->usage_mb = usage_mb;
  out->usage_gb_rated = tr_usage_gb_rounded(usage_mb);
  out->overage_gb = tr_overage_gb(usage_mb, a->included_gb);

  out->plan_charge = tr_prorated_plan_charge(a->plan_fee, a->prev_plan_fee, a->plan_change_day);
  out->line_discount = tr_multi_line_discount(out->plan_charge, a->line_count);
  out->recurring = tr_money(out->plan_charge - out->line_discount);
  out->overage_charges = tr_money(tr_rate_overage(usage_mb, a->included_gb));
  out->suspension_credit =
      tr_suspension_credit(a->plan_fee, a->suspension_start_day, a->suspension_end_day);
  out->promo_credit = tr_promo_credit(a->promo_amount, a->promo_issued_on, period);
  out->late_fee = tr_late_fee(a->prior_balance, a->prior_due_date, period);

  subtotal = out->recurring + out->overage_charges + out->late_fee - out->suspension_credit -
             out->promo_credit;
  if (subtotal < 0.0) subtotal = 0.0;
  out->subtotal = tr_money(subtotal);

  tr_rates_for_province(a->province, &rates);
  out->loyalty_discount = tr_loyalty_discount(out->subtotal, a->loyalty_pct);
  out->federal_tax = tr_federal_tax(out->subtotal, &rates);
  out->provincial_tax = tr_provincial_tax(out->subtotal, out->loyalty_discount, &rates);
  strncpy(out->federal_label, rates.federal_label, TR_LABEL_LEN - 1);
  out->federal_label[TR_LABEL_LEN - 1] = '\0';
  strncpy(out->provincial_label, rates.provincial_label, TR_LABEL_LEN - 1);
  out->provincial_label[TR_LABEL_LEN - 1] = '\0';
  out->total = tr_money(out->subtotal - out->loyalty_discount + out->federal_tax +
                        out->provincial_tax);
}

const char *tr_rules_version(void) {
  return TR_RULES_VERSION;
}
