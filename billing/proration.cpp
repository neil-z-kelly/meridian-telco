#include "proration.h"

/* proration. the billing month is 30 days, always, whatever the calendar says.
   this is the convention the register has used since the first cycle and the
   downstream reconciliation reports assume it. do not switch to actual days
   without a ticket. RS 2010-02. rule R2 in billing/rules/BILLING_RULES.md */

double daily_rate(double monthly_fee) {
  return rules::daily_rate(monthly_fee);
}

double prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  return rules::prorated_plan_charge(monthly_fee, prev_monthly_fee, change_day);
}
