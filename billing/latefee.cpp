#include "latefee.h"

/* late fee. ten day grace after the due date, then 1.5% of the balance still
   outstanding when the next cycle opens. rule R5 in billing/rules/BILLING_RULES.md */

long days_past_due(const std::string &due_date, const std::string &period) {
  return rules::days_past_due(due_date, period);
}

double late_fee(double prior_balance, const std::string &due_date, const std::string &period) {
  return rules::late_fee(prior_balance, due_date, period);
}
