#include "proration.h"
#include "money.h"

/* proration. the billing month is 30 days, always, whatever the calendar says.
   this is the convention the register has used since the first cycle and the
   downstream reconciliation reports assume it. do not switch to actual days
   without a ticket. RS 2010-02 */

double daily_rate(double monthly_fee) {
  return monthly_fee / (double)BILLING_MONTH_DAYS;
}

double prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day) {
  if (change_day <= 0) return money(monthly_fee);
  int days_on_old = change_day - 1;
  if (days_on_old < 0) days_on_old = 0;
  if (days_on_old > BILLING_MONTH_DAYS) days_on_old = BILLING_MONTH_DAYS;
  int days_on_new = BILLING_MONTH_DAYS - days_on_old;
  double old_part = money(daily_rate(prev_monthly_fee) * (double)days_on_old);
  double new_part = money(daily_rate(monthly_fee) * (double)days_on_new);
  return money(old_part + new_part);
}
