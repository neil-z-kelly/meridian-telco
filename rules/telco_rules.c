#include "telco_rules.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* --- dates ---------------------------------------------------------------
   dates arrive as YYYY-MM-DD strings out of the account records, periods as
   YYYY-MM. */

typedef struct {
  int y;
  int m;
  int d;
  int ok;
} TrDate;

static int digits2(const char *s) { return (s[0] - '0') * 10 + (s[1] - '0'); }

static TrDate parse_date(const char *s) {
  TrDate out;
  out.y = 0;
  out.m = 0;
  out.d = 0;
  out.ok = 0;
  if (s == 0 || strlen(s) < 10) return out;
  out.y = atoi(s) ;
  out.m = digits2(s + 5);
  out.d = digits2(s + 8);
  out.ok = out.y > 0 && out.m > 0 && out.d > 0;
  return out;
}

static TrDate period_start(const char *period) {
  TrDate out;
  out.y = 0;
  out.m = 0;
  out.d = 1;
  out.ok = 0;
  if (period == 0 || strlen(period) < 7) return out;
  out.y = atoi(period);
  out.m = digits2(period + 5);
  out.ok = out.y > 0 && out.m > 0;
  return out;
}

/* days since 1970-01-01, howard hinnant's days_from_civil. */
static long serial_day(TrDate dt) {
  int y = dt.y;
  long era;
  unsigned yoe, doy, doe;
  y -= dt.m <= 2;
  era = (y >= 0 ? y : y - 399) / 400;
  yoe = (unsigned)(y - era * 400);
  doy = (153 * (dt.m + (dt.m > 2 ? -3 : 9)) + 2) / 5 + dt.d - 1;
  doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}

/* --- rounding ------------------------------------------------------------ */

double tr_money(double amount) { return floor(amount * 100.0 + 0.5) / 100.0; }

/* --- rating -------------------------------------------------------------- */

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

/* --- proration ----------------------------------------------------------- */

double tr_daily_rate(double monthly_fee) {
  return monthly_fee / (double)TR_BILLING_MONTH_DAYS;
}

double tr_prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  int days_on_old, days_on_new;
  double old_part, new_part;
  if (change_day <= 0) return tr_money(monthly_fee);
  days_on_old = change_day - 1;
  if (days_on_old < 0) days_on_old = 0;
  if (days_on_old > TR_BILLING_MONTH_DAYS) days_on_old = TR_BILLING_MONTH_DAYS;
  days_on_new = TR_BILLING_MONTH_DAYS - days_on_old;
  old_part = tr_money(tr_daily_rate(prev_monthly_fee) * (double)days_on_old);
  new_part = tr_money(tr_daily_rate(monthly_fee) * (double)days_on_new);
  return tr_money(old_part + new_part);
}

/* --- promotional credits ------------------------------------------------- */

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

/* --- suspension ---------------------------------------------------------- */

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

/* --- multi line discount ------------------------------------------------- */

double tr_multi_line_pct(int line_count) {
  if (line_count >= 10) return 10.0;
  if (line_count >= 3) return 5.0;
  return 0.0;
}

double tr_multi_line_discount(double recurring_charge, int line_count) {
  return tr_money(recurring_charge * tr_multi_line_pct(line_count) / 100.0);
}

/* --- late fee ------------------------------------------------------------ */

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

/* --- canadian sales tax -------------------------------------------------- */

static TrTaxRates make_rates(double fed, double prov, const char *fed_label,
                             const char *prov_label) {
  TrTaxRates r;
  r.federal_pct = fed;
  r.provincial_pct = prov;
  r.federal_label = fed_label;
  r.provincial_label = prov_label;
  return r;
}

TrTaxRates tr_rates_for_province(const char *province) {
  const char *p = province ? province : "";
  if (strcmp(p, "BC") == 0) return make_rates(5.0, 7.0, "GST", "PST");
  if (strcmp(p, "AB") == 0) return make_rates(5.0, 0.0, "GST", "");
  if (strcmp(p, "ON") == 0) return make_rates(13.0, 0.0, "HST", "");
  if (strcmp(p, "QC") == 0) return make_rates(5.0, 9.975, "GST", "QST");
  return make_rates(5.0, 0.0, "GST", "");
}

double tr_federal_tax(double pre_discount_amount, TrTaxRates rates) {
  return tr_money(pre_discount_amount * rates.federal_pct / 100.0);
}

double tr_provincial_tax(double pre_discount_amount, double discount, TrTaxRates rates,
                         int tax_base_policy) {
  double base;
  if (rates.provincial_pct <= 0.0) return 0.0;
  if (tax_base_policy == TR_TAX_BASE_PRE_DISCOUNT) {
    base = pre_discount_amount;
  } else {
    base = pre_discount_amount - discount;
  }
  if (base < 0.0) base = 0.0;
  return tr_money(base * rates.provincial_pct / 100.0);
}

/* --- loyalty credit ------------------------------------------------------ */

double tr_loyalty_discount(double amount, double loyalty_pct) {
  return tr_money(amount * loyalty_pct / 100.0);
}

double tr_apply_loyalty(double amount, double loyalty_pct) {
  return tr_money(amount - tr_loyalty_discount(amount, loyalty_pct));
}

/* --- whole invoice ------------------------------------------------------- */

TrPolicy tr_policy_legacy(void) {
  TrPolicy p;
  p.provincial_tax_base = TR_TAX_BASE_POST_DISCOUNT;
  return p;
}

void tr_compute_invoice(const TrInvoiceInput *in, TrPolicy policy, TrInvoice *out) {
  TrTaxRates rates;
  double subtotal;
  long included_mb;

  out->usage_gb_rated = tr_usage_gb_rounded(in->usage_mb);
  out->overage_gb = tr_overage_gb(in->usage_mb, in->included_gb);
  included_mb = in->included_gb * TR_MB_PER_GB;
  out->overage_mb = in->usage_mb > included_mb ? in->usage_mb - included_mb : 0;

  out->plan_charge = tr_prorated_plan_charge(in->plan_fee, in->prev_plan_fee, in->plan_chg_day);
  out->line_discount = tr_multi_line_discount(out->plan_charge, in->line_cnt);
  out->recurring = tr_money(out->plan_charge - out->line_discount);
  out->overage_charges = tr_money(tr_rate_overage(in->usage_mb, in->included_gb));
  out->suspension_credit = tr_suspension_credit(in->plan_fee, in->susp_start, in->susp_end);
  out->promo_credit = tr_promo_credit(in->promo_amt, in->promo_dt, in->period);
  out->late_fee = tr_late_fee(in->prior_bal, in->prior_due, in->period);

  subtotal = out->recurring + out->overage_charges + out->late_fee - out->suspension_credit -
             out->promo_credit;
  if (subtotal < 0.0) subtotal = 0.0;
  out->subtotal = tr_money(subtotal);

  rates = tr_rates_for_province(in->province);
  out->loyalty = tr_loyalty_discount(out->subtotal, in->loyalty_pct);
  out->federal_tax = tr_federal_tax(out->subtotal, rates);
  out->provincial_tax =
      tr_provincial_tax(out->subtotal, out->loyalty, rates, policy.provincial_tax_base);
  out->federal_label = rates.federal_label;
  out->provincial_label = rates.provincial_label;
  out->total = tr_money(out->subtotal - out->loyalty + out->federal_tax + out->provincial_tax);
}
