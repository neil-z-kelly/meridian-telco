#ifndef BILLING_RULES_H
#define BILLING_RULES_H

/* Shared billing rules.

   This file and billing_rules.c are the only implementation of the rules that
   decide what a customer owes. meridian-telco links them into the C++ billing
   binaries; vantage-telco vendors the same two files and calls the compiled
   object through ctypes. Nothing else may restate a rule in another language:
   tools/vendor/check_drift.py in vantage-telco fails that build when the two
   copies stop matching byte for byte.

   Every rule keeps the legacy meridian behaviour. Where vantage-telco had
   drifted, the legacy answer is the one implemented here and the divergence is
   recorded in AMBIGUITIES.md. */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BR_OVERAGE_RATE_PER_GB 10.00
#define BR_MB_PER_GB 1024
#define BR_BILLING_MONTH_DAYS 30
#define BR_LATE_FEE_GRACE_DAYS 10
#define BR_LATE_FEE_PCT 1.5
#define BR_MULTI_LINE_TIER1_LINES 3
#define BR_MULTI_LINE_TIER1_PCT 5.0
#define BR_MULTI_LINE_TIER2_LINES 10
#define BR_MULTI_LINE_TIER2_PCT 10.0

#define BR_LABEL_LEN 8
#define BR_ID_LEN 64
#define BR_DATE_LEN 16

/* Rounds to cents. Every charge line is rounded as it is produced and the
   invoice total is the sum of the rounded lines. */
double br_money(double amount);

/* Rating: whole gigabytes, partial gigabytes round up, flat rate per gigabyte
   over the plan allowance. */
long br_usage_gb_rounded(long usage_mb);
long br_overage_gb(long usage_mb, long included_gb);
double br_rate_overage(long usage_mb, long included_gb);

/* Proration: the billing month is 30 days whatever the calendar says. */
double br_daily_rate(double monthly_fee);
double br_prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day);

/* Promotional credits expire at the end of the cycle they were issued in. */
int br_promo_is_live(const char *issued_on, const char *period);
double br_promo_credit(double amount, const char *issued_on, const char *period);

/* A suspended line keeps its provisioning, so the month is billed in full. */
int br_suspended_days(int start_day, int end_day);
double br_suspension_credit(double monthly_fee, int start_day, int end_day);

/* Multi-line discount off the recurring charge. */
double br_multi_line_pct(int line_count);
double br_multi_line_discount(double recurring_charge, int line_count);

/* Late fee on a balance carried into the cycle. */
long br_days_past_due(const char *due_date, const char *period);
double br_late_fee(double prior_balance, const char *due_date, const char *period);

/* Canadian sales tax. The federal component sits on the pre-discount amount,
   the provincial component on what the customer actually pays. */
typedef struct {
  double federal_pct;
  double provincial_pct;
  char federal_label[BR_LABEL_LEN];
  char provincial_label[BR_LABEL_LEN];
} br_tax_rates;

void br_rates_for_province(const char *province, br_tax_rates *out);
double br_federal_tax(double pre_discount_amount, const br_tax_rates *rates);
double br_provincial_tax(double pre_discount_amount, double discount, const br_tax_rates *rates);

/* Loyalty credit off the subtotal. */
double br_loyalty_discount(double amount, double loyalty_pct);

/* One account, as the rules need to see it. Field widths are fixed so the
   struct can be mirrored by ctypes without a code generator. */
typedef struct {
  char acct_id[BR_ID_LEN];
  char province[BR_LABEL_LEN];
  double plan_fee;
  long included_gb;
  double prev_plan_fee;
  int plan_chg_day;
  int line_cnt;
  double promo_amt;
  char promo_dt[BR_DATE_LEN];
  int susp_start;
  int susp_end;
  double prior_bal;
  char prior_due[BR_DATE_LEN];
  double loyalty_pct;
} br_account;

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
  double loyalty;
  double federal_tax;
  double provincial_tax;
  char federal_label[BR_LABEL_LEN];
  char provincial_label[BR_LABEL_LEN];
  double total;
} br_invoice;

/* Assembles one invoice for one account and one usage record. Both services
   go through this so neither can assemble the lines in its own order. */
void br_compute_invoice(const br_account *account, long usage_mb, const char *period,
                        br_invoice *out);

/* Fills account fields that are not set by the caller. */
void br_account_init(br_account *account);

/* Copies src into a fixed-width field, truncating and always terminating. */
void br_set_field(char *field, size_t size, const char *src);

#ifdef __cplusplus
}
#endif

#endif
