#include "invoice.h"
#include "discounts.h"
#include "latefee.h"
#include "lines.h"
#include "money.h"
#include "promo.h"
#include "proration.h"
#include "rating.h"
#include "suspension.h"
#include "tax.h"

/* one invoice for one account and one usage record. the register and the
   invoice api both come through here so they cannot drift from each other. */

Invoice compute_invoice(const Account &a, const UsageRec &u) {
  Invoice inv;
  inv.acct_id = a.acct_id;
  inv.billing_ref = a.billing_ref;
  inv.period = u.period;
  inv.province = a.province;
  inv.usage_mb = u.usage_mb;
  inv.usage_gb_rated = usage_gb_rounded(u.usage_mb);
  inv.overage_gb = overage_gb(u.usage_mb, a.included_gb);

  inv.plan_charge = prorated_plan_charge(a.plan_fee, a.prev_plan_fee, a.plan_chg_day);
  inv.line_discount = multi_line_discount(inv.plan_charge, a.line_cnt);
  inv.recurring = money(inv.plan_charge - inv.line_discount);
  inv.overage_charges = money(rate_overage(u.usage_mb, a.included_gb));
  inv.suspension_credit_amt = suspension_credit(a.plan_fee, a.susp_start, a.susp_end);
  inv.promo_credit_amt = promo_credit(a.promo_amt, a.promo_dt, u.period);
  inv.late_fee_amt = late_fee(a.prior_bal, a.prior_due, u.period);

  double subtotal = inv.recurring + inv.overage_charges + inv.late_fee_amt
                    - inv.suspension_credit_amt - inv.promo_credit_amt;
  if (subtotal < 0.0) subtotal = 0.0;
  inv.subtotal = money(subtotal);

  TaxRates rates = rates_for_province(a.province);
  inv.loyalty_amt = loyalty_discount(inv.subtotal, a.loyalty_pct);
  inv.federal_tax_amt = federal_tax(inv.subtotal, rates);
  inv.provincial_tax_amt = provincial_tax(inv.subtotal, inv.loyalty_amt, rates);
  inv.federal_label = rates.federal_label;
  inv.provincial_label = rates.provincial_label;
  inv.total = money(inv.subtotal - inv.loyalty_amt + inv.federal_tax_amt + inv.provincial_tax_amt);
  return inv;
}
