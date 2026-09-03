#ifndef TELCO_RULES_H
#define TELCO_RULES_H

/* telco-rules: the canonical billing rules for the meridian and vantage
   estates. one implementation, in C89 with no dependencies, so both the
   legacy C++ register and the python service execute exactly the same
   arithmetic on exactly the same doubles.

   consumers:
     meridian-telco  compiled straight into bin/billing-run and bin/invoice-api
     vantage-telco   loaded with ctypes from shared/telco-rules (vendored copy,
                     checksum gated against this file)

   every rule below is the meridian behaviour, which is the system of record,
   except where noted. the one rule the two estates are allowed to disagree on
   is the provincial tax base, and that disagreement is expressed as an
   explicit policy flag, never as two implementations. */

#ifdef __cplusplus
extern "C" {
#endif

/* --- rounding ------------------------------------------------------------
   the register has always been assembled from lines that are each rounded to
   cents as they are produced; the total is their sum. RS 2011-03 */
double tr_money(double amount);

/* --- rating --------------------------------------------------------------
   whole gigabytes, partial gigabytes round up, $10 a gigabyte over the plan
   allowance, flat, no tiers. */
#define TR_OVERAGE_RATE_PER_GB 10.0
#define TR_MB_PER_GB 1024L

long tr_usage_gb_rounded(long usage_mb);
long tr_overage_gb(long usage_mb, long included_gb);
double tr_rate_overage(long usage_mb, long included_gb);

/* --- proration -----------------------------------------------------------
   the billing month is 30 days, always, whatever the calendar says. the
   downstream reconciliation reports assume it. RS 2010-02 */
#define TR_BILLING_MONTH_DAYS 30

double tr_daily_rate(double monthly_fee);
double tr_prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day);

/* --- promotional credits -------------------------------------------------
   a credit expires at the end of the cycle it was issued in. care wanted a
   rolling 30 days once, finance said no, the cycle is the unit. RS 2012-08 */
int tr_promo_is_live(const char *issued_on, const char *period);
double tr_promo_credit(double amount, const char *issued_on, const char *period);

/* --- suspension ----------------------------------------------------------
   a suspended line keeps its number, its provisioning and its place on the
   switch, so the month is billed in full and there is no partial month
   credit. RS 2013-05 */
int tr_suspended_days(int start_day, int end_day);
double tr_suspension_credit(double monthly_fee, int start_day, int end_day);

/* --- multi line discount -------------------------------------------------
   3-9 lines 5% off recurring, 10+ lines 10%. both estates already agreed. */
double tr_multi_line_pct(int line_count);
double tr_multi_line_discount(double recurring_charge, int line_count);

/* --- late fee ------------------------------------------------------------
   ten day grace after the due date, then 1.5% of the balance still
   outstanding when the next cycle opens. both estates already agreed. */
#define TR_LATE_FEE_GRACE_DAYS 10
#define TR_LATE_FEE_PCT 1.5

long tr_days_past_due(const char *due_date, const char *period);
double tr_late_fee(double prior_balance, const char *due_date, const char *period);

/* --- canadian sales tax --------------------------------------------------
   GST, and HST where the province harmonized, is assessed on the charge
   before any loyalty discount. CRA treats the discount as a goodwill credit
   and not a reduction of consideration; both estates already agreed.

   the provincial component (PST, QST) is the open question. see
   tr_provincial_tax below. */
typedef struct {
  double federal_pct;
  double provincial_pct;
  const char *federal_label;
  const char *provincial_label;
} TrTaxRates;

TrTaxRates tr_rates_for_province(const char *province);
double tr_federal_tax(double pre_discount_amount, TrTaxRates rates);

/* provincial tax base. UNRESOLVED - finance owns this one.

   TR_TAX_BASE_POST_DISCOUNT  meridian, signed off 2010: PST and QST are
                              assessed on what the customer actually pays, so
                              the loyalty credit comes off first. RS 2010-06
   TR_TAX_BASE_PRE_DISCOUNT   vantage: the estate taxes what was invoiced, not
                              what was collected, so the loyalty credit does
                              not reduce the base.

   both are defensible and the two estates currently run with different
   settings on purpose. do not resolve this in code. */
#define TR_TAX_BASE_POST_DISCOUNT 0
#define TR_TAX_BASE_PRE_DISCOUNT 1

double tr_provincial_tax(double pre_discount_amount, double discount, TrTaxRates rates,
                         int tax_base_policy);

/* --- loyalty credit ------------------------------------------------------ */
double tr_loyalty_discount(double amount, double loyalty_pct);
double tr_apply_loyalty(double amount, double loyalty_pct);

/* --- whole invoice -------------------------------------------------------
   the order the lines are assembled in, and the points at which they are
   rounded, are themselves a rule. both estates build the invoice here so the
   assembly cannot drift either. */
typedef struct {
  int provincial_tax_base; /* TR_TAX_BASE_* */
} TrPolicy;

TrPolicy tr_policy_legacy(void);

typedef struct {
  const char *province;
  const char *period;
  double plan_fee;
  double prev_plan_fee;
  int plan_chg_day;
  long included_gb;
  int line_cnt;
  long usage_mb;
  double promo_amt;
  const char *promo_dt;
  int susp_start;
  int susp_end;
  double prior_bal;
  const char *prior_due;
  double loyalty_pct;
} TrInvoiceInput;

typedef struct {
  long usage_gb_rated;
  long overage_gb;
  long overage_mb;
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
  double total;
  const char *federal_label;
  const char *provincial_label;
} TrInvoice;

void tr_compute_invoice(const TrInvoiceInput *in, TrPolicy policy, TrInvoice *out);

#ifdef __cplusplus
}
#endif

#endif
