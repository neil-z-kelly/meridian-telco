#ifndef MERIDIAN_LINES_H
#define MERIDIAN_LINES_H

#include "../rules/telco_rules.h"

/* multi line discount off the recurring charge. 3 to 9 lines 5%, 10 or more
   10%. same schedule finance publishes in the rate card, and the same schedule
   vantage already ran. implemented once in rules/telco_rules.c. */

inline double multi_line_pct(int line_count) { return tr_multi_line_pct(line_count); }
inline double multi_line_discount(double recurring_charge, int line_count) {
  return tr_multi_line_discount(recurring_charge, line_count);
}

#endif
