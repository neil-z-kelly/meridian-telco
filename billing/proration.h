#ifndef MERIDIAN_PRORATION_H
#define MERIDIAN_PRORATION_H

#include "rules.h"

#define BILLING_MONTH_DAYS rules::BILLING_MONTH_DAYS

double daily_rate(double monthly_fee);
double prorated_plan_charge(double monthly_fee, double prev_monthly_fee, int change_day);

#endif
