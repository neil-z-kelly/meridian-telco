#ifndef MERIDIAN_LATEFEE_H
#define MERIDIAN_LATEFEE_H

#include <string>
#include "../rules/telco_rules.h"

/* ten day grace after the due date, then 1.5% of the balance. both estates
   already ran this schedule. implemented once in rules/telco_rules.c. */

#define LATE_FEE_GRACE_DAYS TR_LATE_FEE_GRACE_DAYS
#define LATE_FEE_PCT TR_LATE_FEE_PCT

inline long days_past_due(const std::string &due_date, const std::string &period) {
  return tr_days_past_due(due_date.c_str(), period.c_str());
}
inline double late_fee(double prior_balance, const std::string &due_date,
                       const std::string &period) {
  return tr_late_fee(prior_balance, due_date.c_str(), period.c_str());
}

#endif
