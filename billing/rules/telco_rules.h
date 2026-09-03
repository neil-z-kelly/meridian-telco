#ifndef TELCO_RULES_H
#define TELCO_RULES_H

/* Shared billing rules.

   One implementation of every rule the meridian and vantage estates disagreed
   on. This file and telco_rules.c are the only place the rules exist: the
   meridian C++ links them directly, vantage vendors the source and calls the
   compiled object through ctypes.

   Amounts are doubles rounded to cents by tr_money() as each line is produced;
   the invoice total is the sum of the rounded lines. Dates are YYYY-MM-DD and
   periods YYYY-MM, both NUL terminated; an empty or unparsable date means the
   rule that reads it does not fire.

   Behaviour is the legacy (meridian) behaviour wherever the two estates
   disagreed. RULES.md records each rule, the variant that lost, and the cases
   still waiting on a decision. */

#ifdef __cplusplus
extern "C" {
#endif

#define TR_RULES_VERSION "1.0.0"

#define TR_OVERAGE_RATE_PER_GB 10.00
#define TR_BILLING_MONTH_DAYS 30
#define TR_LATE_FEE_GRACE_DAYS 10
#define TR_LATE_FEE_PCT 1.5
#define TR_MB_PER_GB 1024

#define TR_PROVINCE_LEN 8
#define TR_DATE_LEN 16
#define TR_PERIOD_LEN 12
#define TR_LABEL_LEN 8

/* rounding ---------------------------------------------------------------- */

double tr_money(double amount);

/* rating ------------------------------------------------------------------ */

long tr_usage_gb_rounded(long usage_mb);
long tr_overage_gb(long usage_mb, long included_gb);
double tr_rate_overage(long usage_mb, long included_gb);

/* proration --------------------------------------------------------------- */

/* period is accepted so a caller can pass the cycle it is billing; the legacy
   rule prices a partial month on a fixed 30 day month and ignores it. */
double tr_daily_rate(double monthly_fee, const char *period);
double tr_prorated_plan_charge(double monthly_fee, double prev_monthly_fee,
                               int change_day, const char *period);

/* multi line -------------------------------------------------------------- */

double tr_multi_line_pct(int line_count);
double tr_multi_line_discount(double recurring_charge, int line_count);

/* promotional credit ------------------------------------------------------ */

int tr_promo_is_live(const char *issued_on, const char *period);
double tr_promo_credit(double amount, const char *issued_on, const char *period);

/* suspension -------------------------------------------------------------- */

int tr_suspended_days(int start_day, int end_day);
double tr_suspension_credit(double monthly_fee, int start_day, int end_day,
                            const char *period);

/* late fee ---------------------------------------------------------------- */

long tr_days_past_due(const char *due_date, const char *period);
double tr_late_fee(double prior_balance, const char *due_date, const char *period);

/* tax and loyalty --------------------------------------------------------- */

typedef struct {
  double federal_pct;    /* GST, or HST where the province is harmonized */
  double provincial_pct; /* PST or QST, zero in a harmonized province */
  char federal_label[TR_LABEL_LEN];
  char provincial_label[TR_LABEL_LEN];
} TrTaxRates;

void tr_rates_for_province(const char *province, TrTaxRates *out);
double tr_federal_tax(double pre_discount_amount, const TrTaxRates *rates);
double tr_provincial_tax(double pre_discount_amount, double discount,
                         const TrTaxRates *rates);
double tr_loyalty_discount(double amount, double loyalty_pct);

/* whole invoice ----------------------------------------------------------- */

typedef struct {
  char province[TR_PROVINCE_LEN];
  double plan_fee;
  long included_gb;
  double prev_plan_fee;
  int plan_chg_day;
  int line_cnt;
  double promo_amt;
  char promo_dt[TR_DATE_LEN];
  int susp_start;
  int susp_end;
  double prior_bal;
  char prior_due[TR_DATE_LEN];
  double loyalty_pct;
} TrAccount;

typedef struct {
  char period[TR_PERIOD_LEN];
  long usage_mb;
} TrUsage;

typedef struct {
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
} TrInvoice;

void tr_compute_invoice(const TrAccount *account, const TrUsage *usage, TrInvoice *out);

#ifdef __cplusplus
}
#endif

#endif
