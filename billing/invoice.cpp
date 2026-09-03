#include "invoice.h"
#include "policy.h"
#include "../rules/telco_rules.h"

/* one invoice for one account and one usage record. the register and the
   invoice api both come through here so they cannot drift from each other,
   and the arithmetic itself is tr_compute_invoice in rules/telco_rules.c so
   meridian and vantage cannot drift from each other either. */

Invoice compute_invoice(const Account &a, const UsageRec &u) {
  TrInvoiceInput in;
  in.province = a.province.c_str();
  in.period = u.period.c_str();
  in.plan_fee = a.plan_fee;
  in.prev_plan_fee = a.prev_plan_fee;
  in.plan_chg_day = a.plan_chg_day;
  in.included_gb = a.included_gb;
  in.line_cnt = a.line_cnt;
  in.usage_mb = u.usage_mb;
  in.promo_amt = a.promo_amt;
  in.promo_dt = a.promo_dt.c_str();
  in.susp_start = a.susp_start;
  in.susp_end = a.susp_end;
  in.prior_bal = a.prior_bal;
  in.prior_due = a.prior_due.c_str();
  in.loyalty_pct = a.loyalty_pct;

  TrInvoice r;
  tr_compute_invoice(&in, meridian_policy(), &r);

  Invoice inv;
  inv.acct_id = a.acct_id;
  inv.billing_ref = a.billing_ref;
  inv.period = u.period;
  inv.province = a.province;
  inv.usage_mb = u.usage_mb;
  inv.usage_gb_rated = r.usage_gb_rated;
  inv.overage_gb = r.overage_gb;
  inv.plan_charge = r.plan_charge;
  inv.line_discount = r.line_discount;
  inv.recurring = r.recurring;
  inv.overage_charges = r.overage_charges;
  inv.suspension_credit_amt = r.suspension_credit;
  inv.promo_credit_amt = r.promo_credit;
  inv.late_fee_amt = r.late_fee;
  inv.subtotal = r.subtotal;
  inv.loyalty_amt = r.loyalty;
  inv.federal_tax_amt = r.federal_tax;
  inv.provincial_tax_amt = r.provincial_tax;
  inv.federal_label = r.federal_label;
  inv.provincial_label = r.provincial_label;
  inv.total = r.total;
  return inv;
}
