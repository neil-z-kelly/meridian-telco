#include "lines.h"
#include "money.h"

double multi_line_pct(int line_count) {
  if (line_count >= 10) return 10.0;
  if (line_count >= 3) return 5.0;
  return 0.0;
}

double multi_line_discount(double recurring_charge, int line_count) {
  return money(recurring_charge * multi_line_pct(line_count) / 100.0);
}
