#include "invoice.h"
#include "rules/telco_rules.h"

#include <cstring>

/* one invoice for one account and one usage record. the register and the
   invoice api both come through here so they cannot drift from each other.

   every rule is in billing/rules/telco_rules.c, the shared library vantage
   vendors, so the two estates cannot drift from each other either. */

static void copy_field(char *dst, size_t cap, const std::string &src) {
  std::strncpy(dst, src.c_str(), cap - 1);
  dst[cap - 1] = '\0';
}

Invoice compute_invoice(const Account &a, const UsageRec &u) {
  TrAccount ta;
  TrUsage tu;
  TrInvoice ti;
  std::memset(&ta, 0, sizeof(ta));
  std::memset(&tu, 0, sizeof(tu));
  copy_field(ta.province, TR_PROVINCE_LEN, a.province);
  ta.plan_fee = a.plan_fee;
  ta.included_gb = a.included_gb;
  ta.prev_plan_fee = a.prev_plan_fee;
  ta.plan_chg_day = a.plan_chg_day;
  ta.line_cnt = a.line_cnt;
  ta.promo_amt = a.promo_amt;
  copy_field(ta.promo_dt, TR_DATE_LEN, a.promo_dt);
  ta.susp_start = a.susp_start;
  ta.susp_end = a.susp_end;
  ta.prior_bal = a.prior_bal;
  copy_field(ta.prior_due, TR_DATE_LEN, a.prior_due);
  ta.loyalty_pct = a.loyalty_pct;
  copy_field(tu.period, TR_PERIOD_LEN, u.period);
  tu.usage_mb = u.usage_mb;

  tr_compute_invoice(&ta, &tu, &ti);

  Invoice inv;
  inv.acct_id = a.acct_id;
  inv.billing_ref = a.billing_ref;
  inv.period = u.period;
  inv.province = a.province;
  inv.usage_mb = u.usage_mb;
  inv.usage_gb_rated = ti.usage_gb_rated;
  inv.overage_gb = ti.overage_gb;
  inv.plan_charge = ti.plan_charge;
  inv.line_discount = ti.line_discount;
  inv.recurring = ti.recurring;
  inv.overage_charges = ti.overage_charges;
  inv.suspension_credit_amt = ti.suspension_credit;
  inv.promo_credit_amt = ti.promo_credit;
  inv.late_fee_amt = ti.late_fee;
  inv.subtotal = ti.subtotal;
  inv.loyalty_amt = ti.loyalty_discount;
  inv.federal_tax_amt = ti.federal_tax;
  inv.provincial_tax_amt = ti.provincial_tax;
  inv.federal_label = ti.federal_label;
  inv.provincial_label = ti.provincial_label;
  inv.total = ti.total;
  return inv;
}
