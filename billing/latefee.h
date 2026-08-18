#ifndef MERIDIAN_LATEFEE_H
#define MERIDIAN_LATEFEE_H

#include <string>

#define LATE_FEE_GRACE_DAYS 10
#define LATE_FEE_PCT 1.5

long days_past_due(const std::string &due_date, const std::string &period);
double late_fee(double prior_balance, const std::string &due_date, const std::string &period);

#endif
