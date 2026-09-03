#include "invoice.h"
#include "rules/billing_rules.h"

/* one invoice for one account and one usage record. the register and the
   invoice api both come through here, and the arithmetic itself comes out of
   billing/rules/billing_rules.c so vantage-telco cannot drift from it. */

static br_account to_rules_account(const Account &a) {
  br_account out;
  br_account_init(&out);
  br_set_field(out.acct_id, sizeof(out.acct_id), a.acct_id.c_str());
  br_set_field(out.province, sizeof(out.province), a.province.c_str());
  out.plan_fee = a.plan_fee;
  out.included_gb = a.included_gb;
  out.prev_plan_fee = a.prev_plan_fee;
  out.plan_chg_day = a.plan_chg_day;
  out.line_cnt = a.line_cnt;
  out.promo_amt = a.promo_amt;
  br_set_field(out.promo_dt, sizeof(out.promo_dt), a.promo_dt.c_str());
  out.susp_start = a.susp_start;
  out.susp_end = a.susp_end;
  out.prior_bal = a.prior_bal;
  br_set_field(out.prior_due, sizeof(out.prior_due), a.prior_due.c_str());
  out.loyalty_pct = a.loyalty_pct;
  return out;
}

Invoice compute_invoice(const Account &a, const UsageRec &u) {
  br_account ra = to_rules_account(a);
  br_invoice ri;
  br_compute_invoice(&ra, u.usage_mb, u.period.c_str(), &ri);

  Invoice inv;
  inv.acct_id = a.acct_id;
  inv.billing_ref = a.billing_ref;
  inv.period = u.period;
  inv.province = a.province;
  inv.usage_mb = ri.usage_mb;
  inv.usage_gb_rated = ri.usage_gb_rated;
  inv.overage_gb = ri.overage_gb;
  inv.plan_charge = ri.plan_charge;
  inv.line_discount = ri.line_discount;
  inv.recurring = ri.recurring;
  inv.overage_charges = ri.overage_charges;
  inv.suspension_credit_amt = ri.suspension_credit;
  inv.promo_credit_amt = ri.promo_credit;
  inv.late_fee_amt = ri.late_fee;
  inv.subtotal = ri.subtotal;
  inv.loyalty_amt = ri.loyalty;
  inv.federal_tax_amt = ri.federal_tax;
  inv.provincial_tax_amt = ri.provincial_tax;
  inv.federal_label = ri.federal_label;
  inv.provincial_label = ri.provincial_label;
  inv.total = ri.total;
  return inv;
}
