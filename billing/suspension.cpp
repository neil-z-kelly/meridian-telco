#include "suspension.h"
#include "rules.h"

/* suspension and reactivation. a suspended line keeps its number, its
   provisioning and its place on the switch, so the month is billed in full.
   there is no partial month credit for suspension. RS 2013-05.
   rule R10, RULE_AMBIGUOUS in billing/rules/BILLING_RULES.md */

int suspended_days(int start_day, int end_day) {
  return rules::suspended_days(start_day, end_day);
}

double suspension_credit(double monthly_fee, int start_day, int end_day) {
  return rules::suspension_credit(monthly_fee, start_day, end_day);
}
