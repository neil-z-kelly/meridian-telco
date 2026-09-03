#include "promo.h"
#include "rules.h"

/* promotional credits expire at the end of the billing cycle they were issued
   in. a credit issued in june is a june credit, it does not roll into july.
   care wanted 30 days once, finance said no, the cycle is the unit. RS 2012-08.
   rule R9, RULE_AMBIGUOUS in billing/rules/BILLING_RULES.md */

bool promo_is_live(const std::string &issued_on, const std::string &period) {
  return rules::promo_is_live(issued_on, period);
}

double promo_credit(double amount, const std::string &issued_on, const std::string &period) {
  return rules::promo_credit(amount, issued_on, period);
}
