#ifndef MERIDIAN_MONEY_H
#define MERIDIAN_MONEY_H

#include "rules.h"

/* every charge line is rounded to cents as it is produced. the register has
   always been assembled from rounded lines, the total is just their sum.
   RS 2011-03. rule R1 in billing/rules/BILLING_RULES.md */

inline double money(double amount) {
  return rules::money(amount);
}

#endif
