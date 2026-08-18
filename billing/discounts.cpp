#include "discounts.h"
#include "money.h"

/* loyalty credit. it comes off the subtotal, before the provincial component of
   the tax is worked out. see billing/tax.cpp for what that means in practice.
   finance signed off on this in 2010, do not change without a ticket. */

double loyalty_discount(double amount, double loyalty_pct) {
  return money(amount * loyalty_pct / 100.0);
}

double apply_loyalty(double amount, double loyalty_pct) {
  return money(amount - loyalty_discount(amount, loyalty_pct));
}
