#ifndef MERIDIAN_TAX_H
#define MERIDIAN_TAX_H

#include <string>
#include "rules.h"

typedef rules::TaxRates TaxRates;

TaxRates rates_for_province(const std::string &province);

double federal_tax(double pre_discount_amount, const TaxRates &rates);
double provincial_tax(double pre_discount_amount, double discount, const TaxRates &rates);

#endif
