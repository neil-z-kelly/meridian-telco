#ifndef MERIDIAN_PROMO_H
#define MERIDIAN_PROMO_H

#include <string>

bool promo_is_live(const std::string &issued_on, const std::string &period);
double promo_credit(double amount, const std::string &issued_on, const std::string &period);

#endif
