#ifndef MERIDIAN_INVOICE_H
#define MERIDIAN_INVOICE_H

#include <string>
#include "accounts.h"

struct Invoice {
  std::string acct_id;
  std::string billing_ref;
  std::string period;
  std::string province;
  long usage_mb;
  long usage_gb_rated;
  long overage_gb;
  double plan_charge;
  double line_discount;
  double recurring;
  double overage_charges;
  double suspension_credit_amt;
  double promo_credit_amt;
  double late_fee_amt;
  double subtotal;
  double loyalty_amt;
  double federal_tax_amt;
  double provincial_tax_amt;
  std::string federal_label;
  std::string provincial_label;
  double total;
};

Invoice compute_invoice(const Account &a, const UsageRec &u);

#endif
