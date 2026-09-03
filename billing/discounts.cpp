#include "discounts.h"
#include "rules.h"

/* loyalty credit. it comes off the subtotal, before the provincial component of
   the tax is worked out. see billing/tax.cpp for what that means in practice.
   finance signed off on this in 2010, do not change without a ticket.
   rule R6 in billing/rules/BILLING_RULES.md */

double loyalty_discount(double amount, double loyalty_pct) {
  return rules::loyalty_discount(amount, loyalty_pct);
}

double apply_loyalty(double amount, double loyalty_pct) {
  return rules::apply_loyalty(amount, loyalty_pct);
}
