#ifndef MERIDIAN_SUSPENSION_H
#define MERIDIAN_SUSPENSION_H

#include "../rules/telco_rules.h"

/* a suspended line is billed the full month, no credit. RS 2013-05.
   implemented once in rules/telco_rules.c. */

inline int suspended_days(int start_day, int end_day) {
  return tr_suspended_days(start_day, end_day);
}
inline double suspension_credit(double monthly_fee, int start_day, int end_day) {
  return tr_suspension_credit(monthly_fee, start_day, end_day);
}

#endif
