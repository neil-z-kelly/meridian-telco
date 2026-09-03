#ifndef MERIDIAN_DISCOUNTS_H
#define MERIDIAN_DISCOUNTS_H

#include "../rules/telco_rules.h"

/* loyalty credit off the subtotal. implemented once in rules/telco_rules.c. */

inline double loyalty_discount(double amount, double loyalty_pct) {
  return tr_loyalty_discount(amount, loyalty_pct);
}
inline double apply_loyalty(double amount, double loyalty_pct) {
  return tr_apply_loyalty(amount, loyalty_pct);
}

#endif
