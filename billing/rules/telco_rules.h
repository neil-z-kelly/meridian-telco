/* telco_rules — the shared billing rule library.
 *
 * This is the single implementation of every billing rule meridian and vantage
 * both apply. Meridian links it directly; vantage vendors this source verbatim
 * and calls the compiled object through ctypes. The rules are not to be written
 * a second time in another language: a consumer that cannot link the object is
 * held to the conformance vectors emitted by tr_gen_vectors instead.
 *
 * Plain C89 with no dependency beyond libm, so that every consumer can build it.
 * Amounts are doubles carried in currency units and rounded to cents by
 * tr_money at the point each charge line is produced.
 */
#ifndef TELCO_RULES_H
#define TELCO_RULES_H

#ifdef __cplusplus
extern "C" {
#endif

#define TR_RULES_VERSION "1"

/* Rule constants, all of them the legacy meridian values. */
#define TR_OVERAGE_RATE_PER_GB 10.00
#define TR_MB_PER_GB 1024
#define TR_BILLING_MONTH_DAYS 30
#define TR_LATE_FEE_GRACE_DAYS 10
#define TR_LATE_FEE_PCT 1.5

#define TR_LABEL_LEN 8
#define TR_ID_LEN 32
#define TR_DATE_LEN 16

/* R-ROUND: every charge line is rounded to cents as it is produced; the total
   is the sum of the rounded lines. */
double tr_money(double amount);

/* R-RATE: usage rates in whole gigabytes, partial gigabytes round up, flat
   TR_OVERAGE_RATE_PER_GB per gigabyte over the allowance, no tiers. */
long tr_usage_gb_rounded(long usage_mb);
long tr_overage_gb(long usage_mb, long included_gb);
double tr_rate_overage(long usage_mb, long included_gb);

/* R-PRORATE: the billing month is TR_BILLING_MONTH_DAYS days whatever the
   calendar says. change_day 0 means no mid-cycle change. */
double tr_daily_rate(double monthly_fee);
double tr_prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day);

/* R-LINES: multi-line discount off the recurring charge. */
double tr_multi_line_pct(int line_count);
double tr_multi_line_discount(double recurring_charge, int line_count);

/* R-PROMO: a promotional credit belongs to the cycle it was issued in and does
   not roll forward. Dates are YYYY-MM-DD, periods YYYY-MM. */
int tr_promo_is_live(const char *issued_on, const char *period);
double tr_promo_credit(double amount, const char *issued_on, const char *period);

/* R-SUSPEND: a suspended line keeps its provisioning, so the month is billed in
   full and no partial-month credit is raised. */
int tr_suspended_days(int start_day, int end_day);
double tr_suspension_credit(double monthly_fee, int start_day, int end_day);

/* R-LATEFEE: TR_LATE_FEE_GRACE_DAYS of grace after the due date, then
   TR_LATE_FEE_PCT of the balance still outstanding when the cycle opens. */
long tr_days_past_due(const char *due_date, const char *period);
double tr_late_fee(double prior_balance, const char *due_date, const char *period);

/* R-TAXBASE: GST/HST is assessed on the charge before the loyalty discount;
   PST/QST is assessed on what the customer actually pays, so the loyalty
   discount comes off that base first. */
typedef struct {
  double federal_pct;
  double provincial_pct;
  char federal_label[TR_LABEL_LEN];
  char provincial_label[TR_LABEL_LEN];
} tr_tax_rates;

void tr_rates_for_province(const char *province, tr_tax_rates *out);
double tr_federal_tax(double pre_discount_amount, const tr_tax_rates *rates);
double tr_provincial_tax(double pre_discount_amount, double discount, const tr_tax_rates *rates);

/* R-LOYALTY: the loyalty credit is a percentage of the subtotal and is applied
   after tax, as a credit against the invoice total. */
double tr_loyalty_discount(double amount, double loyalty_pct);

/* One invoice for one account and one usage record. Every consumer goes through
   this so the assembly order cannot drift either. */
typedef struct {
  char province[TR_LABEL_LEN];
  double plan_fee;
  long included_gb;
  double prev_plan_fee;
  int plan_change_day;
  int line_count;
  double promo_amount;
  char promo_issued_on[TR_DATE_LEN];
  int suspension_start_day;
  int suspension_end_day;
  double prior_balance;
  char prior_due_date[TR_DATE_LEN];
  double loyalty_pct;
} tr_account;

typedef struct {
  long usage_mb;
  long usage_gb_rated;
  long overage_gb;
  double plan_charge;
  double line_discount;
  double recurring;
  double overage_charges;
  double suspension_credit;
  double promo_credit;
  double late_fee;
  double subtotal;
  double loyalty_discount;
  double federal_tax;
  double provincial_tax;
  char federal_label[TR_LABEL_LEN];
  char provincial_label[TR_LABEL_LEN];
  double total;
} tr_invoice;

void tr_compute_invoice(const tr_account *account, long usage_mb, const char *period,
                        tr_invoice *out);

const char *tr_rules_version(void);

#ifdef __cplusplus
}
#endif

#endif
