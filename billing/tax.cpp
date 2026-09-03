#include "tax.h"

/* canadian sales tax.

   GST (and HST where the province harmonized) is assessed on the charge before
   any loyalty discount. that part is settled, CRA treats the discount as a
   goodwill credit and not a reduction of consideration. rule R7.

   the provincial component is the one finance ruled on locally: PST and QST are
   assessed on what the customer actually pays, so the loyalty discount comes off
   first. signed off 2010, do not change without a ticket. RS 2010-06.
   rule R8, RULE_AMBIGUOUS in billing/rules/BILLING_RULES.md: the legacy base is
   in force pending a tax compliance ruling. */

TaxRates rates_for_province(const std::string &province) {
  return rules::rates_for_province(province);
}

double federal_tax(double pre_discount_amount, const TaxRates &rates) {
  return rules::federal_tax(pre_discount_amount, rates);
}

double provincial_tax(double pre_discount_amount, double discount, const TaxRates &rates) {
  return rules::provincial_tax(pre_discount_amount, discount, rates);
}
