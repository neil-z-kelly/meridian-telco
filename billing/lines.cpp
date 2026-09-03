#include "lines.h"
#include "rules.h"

/* rule R4 in billing/rules/BILLING_RULES.md */

double multi_line_pct(int line_count) {
  return rules::multi_line_pct(line_count);
}

double multi_line_discount(double recurring_charge, int line_count) {
  return rules::multi_line_discount(recurring_charge, line_count);
}
