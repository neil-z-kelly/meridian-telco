#include "billing_rules.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The rules themselves. See billing_rules.h for the ownership rules around
   this file and AMBIGUITIES.md for the cases nobody has ruled on yet. */

void br_set_field(char *field, size_t size, const char *src) {
  size_t i = 0;
  if (size == 0) return;
  if (src) {
    for (; i + 1 < size && src[i]; i++) field[i] = src[i];
  }
  field[i] = '\0';
}

double br_money(double amount) {
  return floor(amount * 100.0 + 0.5) / 100.0;
}

/* ---- dates ---------------------------------------------------------------
   Dates arrive as YYYY-MM-DD, periods as YYYY-MM. */

typedef struct {
  int y;
  int m;
  int d;
  int ok;
} br_date;

static int br_field_int(const char *s, size_t offset, size_t len) {
  char buf[8];
  size_t i;
  if (len >= sizeof(buf)) return 0;
  for (i = 0; i < len; i++) buf[i] = s[offset + i];
  buf[len] = '\0';
  return atoi(buf);
}

static br_date br_parse_date(const char *s) {
  br_date out;
  out.y = 0; out.m = 0; out.d = 0; out.ok = 0;
  if (!s || strlen(s) < 10) return out;
  out.y = br_field_int(s, 0, 4);
  out.m = br_field_int(s, 5, 2);
  out.d = br_field_int(s, 8, 2);
  out.ok = out.y > 0 && out.m > 0 && out.d > 0;
  return out;
}

static br_date br_period_start(const char *period) {
  br_date out;
  out.y = 0; out.m = 0; out.d = 1; out.ok = 0;
  if (!period || strlen(period) < 7) return out;
  out.y = br_field_int(period, 0, 4);
  out.m = br_field_int(period, 5, 2);
  out.ok = out.y > 0 && out.m > 0;
  return out;
}

/* days since 1970-01-01, howard hinnant's civil_from_days in reverse. */
static long br_serial_day(br_date dt) {
  int y = dt.y - (dt.m <= 2);
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (dt.m + (dt.m > 2 ? -3 : 9)) + 2) / 5 + dt.d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}

/* ---- rating --------------------------------------------------------------
   Usage comes off the mediation drop in megabytes. Billing has always been
   done in whole gigabytes, partial gigabytes round up. $10 a gigabyte over the
   plan allowance, flat, no tiers. */

long br_usage_gb_rounded(long usage_mb) {
  long gb = usage_mb / BR_MB_PER_GB;
  if (usage_mb % BR_MB_PER_GB) gb++;
  return gb;
}

long br_overage_gb(long usage_mb, long included_gb) {
  long gb = br_usage_gb_rounded(usage_mb);
  if (gb <= included_gb) return 0;
  return gb - included_gb;
}

double br_rate_overage(long usage_mb, long included_gb) {
  return (double)br_overage_gb(usage_mb, included_gb) * BR_OVERAGE_RATE_PER_GB;
}

/* ---- proration -----------------------------------------------------------
   The billing month is 30 days, always, whatever the calendar says. This is
   the convention the register has used since the first cycle and the
   downstream reconciliation reports assume it. RS 2010-02 */

double br_daily_rate(double monthly_fee) {
  return monthly_fee / (double)BR_BILLING_MONTH_DAYS;
}

double br_prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  int days_on_old;
  int days_on_new;
  double old_part;
  double new_part;
  if (change_day <= 0) return br_money(monthly_fee);
  days_on_old = change_day - 1;
  if (days_on_old < 0) days_on_old = 0;
  if (days_on_old > BR_BILLING_MONTH_DAYS) days_on_old = BR_BILLING_MONTH_DAYS;
  days_on_new = BR_BILLING_MONTH_DAYS - days_on_old;
  old_part = br_money(br_daily_rate(prev_monthly_fee) * (double)days_on_old);
  new_part = br_money(br_daily_rate(monthly_fee) * (double)days_on_new);
  return br_money(old_part + new_part);
}

/* ---- promotional credits -------------------------------------------------
   A credit expires at the end of the billing cycle it was issued in: a credit
   issued in june is a june credit, it does not roll into july. Care wanted 30
   days once, finance said no, the cycle is the unit. RS 2012-08 */

int br_promo_is_live(const char *issued_on, const char *period) {
  br_date issued = br_parse_date(issued_on);
  br_date cycle = br_period_start(period);
  if (!issued.ok || !cycle.ok) return 0;
  return issued.y == cycle.y && issued.m == cycle.m;
}

double br_promo_credit(double amount, const char *issued_on, const char *period) {
  if (amount <= 0.0) return 0.0;
  if (!br_promo_is_live(issued_on, period)) return 0.0;
  return br_money(amount);
}

/* ---- suspension ----------------------------------------------------------
   A suspended line keeps its number, its provisioning and its place on the
   switch, so the month is billed in full. There is no partial month credit for
   suspension. RS 2013-05 */

int br_suspended_days(int start_day, int end_day) {
  if (start_day <= 0 || end_day < start_day) return 0;
  return end_day - start_day + 1;
}

double br_suspension_credit(double monthly_fee, int start_day, int end_day) {
  (void)monthly_fee;
  (void)start_day;
  (void)end_day;
  return 0.0;
}

/* ---- multi-line discount -------------------------------------------------
   3 to 9 lines 5%, 10 or more 10%, the schedule finance publishes in the rate
   card. */

double br_multi_line_pct(int line_count) {
  if (line_count >= BR_MULTI_LINE_TIER2_LINES) return BR_MULTI_LINE_TIER2_PCT;
  if (line_count >= BR_MULTI_LINE_TIER1_LINES) return BR_MULTI_LINE_TIER1_PCT;
  return 0.0;
}

double br_multi_line_discount(double recurring_charge, int line_count) {
  return br_money(recurring_charge * br_multi_line_pct(line_count) / 100.0);
}

/* ---- late fee ------------------------------------------------------------
   Ten day grace after the due date, then 1.5% of the balance still outstanding
   when the next cycle opens. */

long br_days_past_due(const char *due_date, const char *period) {
  br_date due = br_parse_date(due_date);
  br_date cycle = br_period_start(period);
  long days;
  if (!due.ok || !cycle.ok) return 0;
  days = br_serial_day(cycle) - br_serial_day(due);
  return days > 0 ? days : 0;
}

double br_late_fee(double prior_balance, const char *due_date, const char *period) {
  if (prior_balance <= 0.0) return 0.0;
  if (br_days_past_due(due_date, period) <= BR_LATE_FEE_GRACE_DAYS) return 0.0;
  return br_money(prior_balance * BR_LATE_FEE_PCT / 100.0);
}

/* ---- tax -----------------------------------------------------------------
   GST (and HST where the province harmonized) is assessed on the charge before
   any loyalty discount: the CRA treats the discount as a goodwill credit and
   not a reduction of consideration.

   The provincial component is the one finance ruled on locally: PST and QST
   are assessed on what the customer actually pays, so the loyalty discount
   comes off first. Signed off 2010. RS 2010-06 */

static void br_make_rates(br_tax_rates *out, double fed, double prov,
                          const char *fed_label, const char *prov_label) {
  out->federal_pct = fed;
  out->provincial_pct = prov;
  br_set_field(out->federal_label, sizeof(out->federal_label), fed_label);
  br_set_field(out->provincial_label, sizeof(out->provincial_label), prov_label);
}

void br_rates_for_province(const char *province, br_tax_rates *out) {
  const char *p = province ? province : "";
  if (strcmp(p, "BC") == 0) { br_make_rates(out, 5.0, 7.0, "GST", "PST"); return; }
  if (strcmp(p, "AB") == 0) { br_make_rates(out, 5.0, 0.0, "GST", ""); return; }
  if (strcmp(p, "ON") == 0) { br_make_rates(out, 13.0, 0.0, "HST", ""); return; }
  if (strcmp(p, "QC") == 0) { br_make_rates(out, 5.0, 9.975, "GST", "QST"); return; }
  br_make_rates(out, 5.0, 0.0, "GST", "");
}

double br_federal_tax(double pre_discount_amount, const br_tax_rates *rates) {
  return br_money(pre_discount_amount * rates->federal_pct / 100.0);
}

double br_provincial_tax(double pre_discount_amount, double discount, const br_tax_rates *rates) {
  double base;
  if (rates->provincial_pct <= 0.0) return 0.0;
  base = pre_discount_amount - discount;
  if (base < 0.0) base = 0.0;
  return br_money(base * rates->provincial_pct / 100.0);
}

/* ---- loyalty -------------------------------------------------------------
   The credit comes off the subtotal, before the provincial component of the
   tax is worked out. */

double br_loyalty_discount(double amount, double loyalty_pct) {
  return br_money(amount * loyalty_pct / 100.0);
}

/* ---- invoice assembly ---------------------------------------------------- */

void br_account_init(br_account *account) {
  memset(account, 0, sizeof(*account));
  account->line_cnt = 1;
}

void br_compute_invoice(const br_account *a, long usage_mb, const char *period,
                        br_invoice *out) {
  br_tax_rates rates;
  double subtotal;

  memset(out, 0, sizeof(*out));
  out->usage_mb = usage_mb;
  out->usage_gb_rated = br_usage_gb_rounded(usage_mb);
  out->overage_gb = br_overage_gb(usage_mb, a->included_gb);

  out->plan_charge = br_prorated_plan_charge(a->plan_fee, a->prev_plan_fee, a->plan_chg_day);
  out->line_discount = br_multi_line_discount(out->plan_charge, a->line_cnt);
  out->recurring = br_money(out->plan_charge - out->line_discount);
  out->overage_charges = br_money(br_rate_overage(usage_mb, a->included_gb));
  out->suspension_credit = br_suspension_credit(a->plan_fee, a->susp_start, a->susp_end);
  out->promo_credit = br_promo_credit(a->promo_amt, a->promo_dt, period);
  out->late_fee = br_late_fee(a->prior_bal, a->prior_due, period);

  subtotal = out->recurring + out->overage_charges + out->late_fee
             - out->suspension_credit - out->promo_credit;
  if (subtotal < 0.0) subtotal = 0.0;
  out->subtotal = br_money(subtotal);

  br_rates_for_province(a->province, &rates);
  out->loyalty = br_loyalty_discount(out->subtotal, a->loyalty_pct);
  out->federal_tax = br_federal_tax(out->subtotal, &rates);
  out->provincial_tax = br_provincial_tax(out->subtotal, out->loyalty, &rates);
  br_set_field(out->federal_label, sizeof(out->federal_label), rates.federal_label);
  br_set_field(out->provincial_label, sizeof(out->provincial_label), rates.provincial_label);
  out->total = br_money(out->subtotal - out->loyalty + out->federal_tax + out->provincial_tax);
}
