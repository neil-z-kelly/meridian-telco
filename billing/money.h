#ifndef MERIDIAN_MONEY_H
#define MERIDIAN_MONEY_H

#include <cmath>

/* every charge line is rounded to cents as it is produced. the register has
   always been assembled from rounded lines, the total is just their sum.
   RS 2011-03 */

inline double money(double amount) {
  return floor(amount * 100.0 + 0.5) / 100.0;
}

#endif
