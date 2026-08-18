#include "suspension.h"

/* suspension and reactivation. a suspended line keeps its number, its
   provisioning and its place on the switch, so the month is billed in full.
   there is no partial month credit for suspension. RS 2013-05 */

int suspended_days(int start_day, int end_day) {
  if (start_day <= 0 || end_day < start_day) return 0;
  return end_day - start_day + 1;
}

double suspension_credit(double monthly_fee, int start_day, int end_day) {
  (void)monthly_fee;
  (void)start_day;
  (void)end_day;
  return 0.0;
}
