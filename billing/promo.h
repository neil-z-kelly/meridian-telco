#ifndef MERIDIAN_PROMO_H
#define MERIDIAN_PROMO_H

#include <string>
#include "../rules/telco_rules.h"

/* promo credits expire at the end of the cycle they were issued in.
   RS 2012-08. implemented once in rules/telco_rules.c. */

inline bool promo_is_live(const std::string &issued_on, const std::string &period) {
  return tr_promo_is_live(issued_on.c_str(), period.c_str()) != 0;
}
inline double promo_credit(double amount, const std::string &issued_on,
                           const std::string &period) {
  return tr_promo_credit(amount, issued_on.c_str(), period.c_str());
}

#endif
