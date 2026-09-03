#include "rules.h"
#include "datecalc.h"

#include <cmath>

/* shared billing rules, C++ binding. see rules.h and billing/rules/BILLING_RULES.md.
   every formula here has a line-for-line twin in vantage-telco app/billing/rules. */

namespace rules {

const char *const SPEC_VERSION = "1.0.0";

static const AmbiguousRule AMBIGUOUS[] = {
  {"PROVINCIAL_TAX_BASE",
   "PST/QST on subtotal minus loyalty discount",
   "PST/QST on the pre-discount subtotal",
   "finance / tax compliance"},
  {"PROMO_EXPIRY",
   "credit is live only in the calendar cycle it was issued",
   "credit is live for 30 days from issue",
   "marketing / legal (promo terms)"},
  {"SUSPENSION_CREDIT",
   "no credit for suspended days",
   "suspended days credited at the daily rate",
   "legal / customer contracts"},
};

const AmbiguousRule *ambiguous_rules(int *count) {
  *count = (int)(sizeof(AMBIGUOUS) / sizeof(AMBIGUOUS[0]));
  return AMBIGUOUS;
}

/* R1 */
double money(double amount) {
  return floor(amount * 100.0 + 0.5) / 100.0;
}

/* R2 */
double daily_rate(double monthly_fee) {
  return monthly_fee / (double)BILLING_MONTH_DAYS;
}

double prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  if (change_day <= 0) return money(monthly_fee);
  int days_on_old = change_day - 1;
  if (days_on_old < 0) days_on_old = 0;
  if (days_on_old > BILLING_MONTH_DAYS) days_on_old = BILLING_MONTH_DAYS;
  int days_on_new = BILLING_MONTH_DAYS - days_on_old;
  double old_part = money(daily_rate(prev_monthly_fee) * (double)days_on_old);
  double new_part = money(daily_rate(monthly_fee) * (double)days_on_new);
  return money(old_part + new_part);
}

/* R3 */
long usage_gb_rounded(long usage_mb) {
  long gb = usage_mb / MB_PER_GB;
  if (usage_mb % MB_PER_GB) gb++;
  return gb;
}

long overage_gb(long usage_mb, long included_gb) {
  long gb = usage_gb_rounded(usage_mb);
  if (gb <= included_gb) return 0;
  return gb - included_gb;
}

double rate_overage(long usage_mb, long included_gb) {
  return (double)overage_gb(usage_mb, included_gb) * OVERAGE_RATE_PER_GB;
}

/* R4 */
double multi_line_pct(int line_count) {
  if (line_count >= MULTI_LINE_TIER2_MIN_LINES) return MULTI_LINE_TIER2_PCT;
  if (line_count >= MULTI_LINE_TIER1_MIN_LINES) return MULTI_LINE_TIER1_PCT;
  return 0.0;
}

double multi_line_discount(double recurring_charge, int line_count) {
  return money(recurring_charge * multi_line_pct(line_count) / 100.0);
}

/* R5 */
long days_past_due(const std::string &due_date, const std::string &period) {
  BillDate due = parse_date(due_date);
  BillDate cycle = period_start(period);
  if (!due.ok || !cycle.ok) return 0;
  long days = days_between(due, cycle);
  return days > 0 ? days : 0;
}

double late_fee(double prior_balance, const std::string &due_date, const std::string &period) {
  if (prior_balance <= 0.0) return 0.0;
  if (days_past_due(due_date, period) <= LATE_FEE_GRACE_DAYS) return 0.0;
  return money(prior_balance * LATE_FEE_PCT / 100.0);
}

/* R6 */
double loyalty_discount(double amount, double loyalty_pct) {
  return money(amount * loyalty_pct / 100.0);
}

double apply_loyalty(double amount, double loyalty_pct) {
  return money(amount - loyalty_discount(amount, loyalty_pct));
}

/* R7 / R8 */
static TaxRates make(double fed, double prov, const char *fed_label, const char *prov_label) {
  TaxRates r;
  r.federal_pct = fed;
  r.provincial_pct = prov;
  r.federal_label = fed_label;
  r.provincial_label = prov_label;
  return r;
}

TaxRates rates_for_province(const std::string &province) {
  if (province == "BC") return make(5.0, 7.0, "GST", "PST");
  if (province == "AB") return make(5.0, 0.0, "GST", "");
  if (province == "ON") return make(13.0, 0.0, "HST", "");
  if (province == "QC") return make(5.0, 9.975, "GST", "QST");
  return make(5.0, 0.0, "GST", "");
}

double federal_tax(double pre_discount_amount, const TaxRates &rates) {
  return money(pre_discount_amount * rates.federal_pct / 100.0);
}

double provincial_tax(double pre_discount_amount, double discount, const TaxRates &rates) {
  if (rates.provincial_pct <= 0.0) return 0.0;
  double base = pre_discount_amount - discount;
  if (base < 0.0) base = 0.0;
  return money(base * rates.provincial_pct / 100.0);
}

/* R9 */
bool promo_is_live(const std::string &issued_on, const std::string &period) {
  BillDate issued = parse_date(issued_on);
  BillDate cycle = period_start(period);
  if (!issued.ok || !cycle.ok) return false;
  return issued.y == cycle.y && issued.m == cycle.m;
}

double promo_credit(double amount, const std::string &issued_on, const std::string &period) {
  if (amount <= 0.0) return 0.0;
  if (!promo_is_live(issued_on, period)) return 0.0;
  return money(amount);
}

/* R10 */
int suspended_days(int start_day, int end_day) {
  if (start_day <= 0 || end_day < start_day) return 0;
  return end_day - start_day + 1;
}

double suspension_credit(double monthly_fee, int start_day, int end_day) {
  (void)monthly_fee;
  (void)start_day;
  (void)end_day;
  return 0.0;
}

} /* namespace rules */
