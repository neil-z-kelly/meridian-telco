#ifndef MERIDIAN_PRORATION_H
#define MERIDIAN_PRORATION_H

#include "../rules/telco_rules.h"

/* 30 day billing month, mid cycle plan changes split on it. RS 2010-02.
   implemented once in rules/telco_rules.c. */

#define BILLING_MONTH_DAYS TR_BILLING_MONTH_DAYS

inline double daily_rate(double monthly_fee) { return tr_daily_rate(monthly_fee); }
inline double prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  return tr_prorated_plan_charge(monthly_fee, prev_monthly_fee, change_day);
}

#endif
