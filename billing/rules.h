#ifndef MERIDIAN_RULES_H
#define MERIDIAN_RULES_H

#include <string>

/* shared billing rules, C++ binding.

   the authoritative rule set is billing/rules/BILLING_RULES.md and
   billing/rules/billing_rules.json. vantage-telco carries the same spec and a
   python binding (app/billing/rules). the constants and formulas here must match
   the spec exactly; billing/rules_test.cpp checks the constants against the json.

   rules marked RULE_AMBIGUOUS below run on the legacy default so the two engines
   agree, but the default is awaiting a human decision. see the spec. */

namespace rules {

extern const char *const SPEC_VERSION;

/* constants */
const int BILLING_MONTH_DAYS = 30;
const long MB_PER_GB = 1024;
const double OVERAGE_RATE_PER_GB = 10.00;
const int MULTI_LINE_TIER1_MIN_LINES = 3;
const double MULTI_LINE_TIER1_PCT = 5.0;
const int MULTI_LINE_TIER2_MIN_LINES = 10;
const double MULTI_LINE_TIER2_PCT = 10.0;
const int LATE_FEE_GRACE_DAYS = 10;
const double LATE_FEE_PCT = 1.5;

/* ambiguity flags. true = implemented on the legacy default pending a ruling. */
const bool RULE_AMBIGUOUS_PROVINCIAL_TAX_BASE = true;
const bool RULE_AMBIGUOUS_PROMO_EXPIRY = true;
const bool RULE_AMBIGUOUS_SUSPENSION_CREDIT = true;

struct AmbiguousRule {
  const char *rule;
  const char *legacy_default;
  const char *alternative;
  const char *decider;
};
const AmbiguousRule *ambiguous_rules(int *count);

/* R1 rounding */
double money(double amount);

/* R2 proration */
double daily_rate(double monthly_fee);
double prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day);

/* R3 overage rating */
long usage_gb_rounded(long usage_mb);
long overage_gb(long usage_mb, long included_gb);
double rate_overage(long usage_mb, long included_gb);

/* R4 multi line discount */
double multi_line_pct(int line_count);
double multi_line_discount(double recurring_charge, int line_count);

/* R5 late fee */
long days_past_due(const std::string &due_date, const std::string &period);
double late_fee(double prior_balance, const std::string &due_date, const std::string &period);

/* R6 loyalty */
double loyalty_discount(double amount, double loyalty_pct);
double apply_loyalty(double amount, double loyalty_pct);

/* R7 / R8 tax */
struct TaxRates {
  double federal_pct;
  double provincial_pct;
  std::string federal_label;
  std::string provincial_label;
};
TaxRates rates_for_province(const std::string &province);
double federal_tax(double pre_discount_amount, const TaxRates &rates);
double provincial_tax(double pre_discount_amount, double discount, const TaxRates &rates); /* RULE_AMBIGUOUS */

/* R9 promo */
bool promo_is_live(const std::string &issued_on, const std::string &period); /* RULE_AMBIGUOUS */
double promo_credit(double amount, const std::string &issued_on, const std::string &period);

/* R10 suspension */
int suspended_days(int start_day, int end_day);
double suspension_credit(double monthly_fee, int start_day, int end_day); /* RULE_AMBIGUOUS */

} /* namespace rules */

#endif
