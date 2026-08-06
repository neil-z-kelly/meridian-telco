#include "discounts.h"

/* loyalty comes off first, tax is charged on what the customer actually pays.
   finance signed off on this in 2010, do not change without a ticket. */

double apply_loyalty(double amount, double loyalty_pct) {
  return amount - (amount * loyalty_pct / 100.0);
}

double apply_tax(double amount, double tax_pct) {
  return amount + (amount * tax_pct / 100.0);
}

double invoice_total(double charges, double loyalty_pct, double tax_pct) {
  double discounted = apply_loyalty(charges, loyalty_pct);
  return apply_tax(discounted, tax_pct);
}
