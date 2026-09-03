#ifndef MERIDIAN_TAX_H
#define MERIDIAN_TAX_H

#include <string>
#include "policy.h"
#include "../rules/telco_rules.h"

/* canadian sales tax. rates and both tax bases live in rules/telco_rules.c;
   which base meridian runs on is declared once, in billing/policy.h. */

struct TaxRates {
  double federal_pct;    /* GST, or HST where the province is harmonized */
  double provincial_pct; /* PST or QST, zero in a harmonized province */
  std::string federal_label;
  std::string provincial_label;
};

inline TaxRates rates_for_province(const std::string &province) {
  TrTaxRates r = tr_rates_for_province(province.c_str());
  TaxRates out;
  out.federal_pct = r.federal_pct;
  out.provincial_pct = r.provincial_pct;
  out.federal_label = r.federal_label;
  out.provincial_label = r.provincial_label;
  return out;
}

inline TrTaxRates to_shared(const TaxRates &rates) {
  TrTaxRates r;
  r.federal_pct = rates.federal_pct;
  r.provincial_pct = rates.provincial_pct;
  r.federal_label = rates.federal_label.c_str();
  r.provincial_label = rates.provincial_label.c_str();
  return r;
}

inline double federal_tax(double pre_discount_amount, const TaxRates &rates) {
  return tr_federal_tax(pre_discount_amount, to_shared(rates));
}

inline double provincial_tax(double pre_discount_amount, double discount, const TaxRates &rates) {
  return tr_provincial_tax(pre_discount_amount, discount, to_shared(rates),
                           meridian_policy().provincial_tax_base);
}

#endif
