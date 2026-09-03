#include "invoice.h"
#include "rules/telco_rules.h"

#include <cstring>

/* one invoice for one account and one usage record. the register and the
   invoice api both come through here, and this is the only place in the C++
   tree that knows a billing rule: everything below the copy into tr_account is
   billing/rules/telco_rules.c, the same source vantage vendors. */

static void copy_field(char *dst, size_t n, const std::string &src) {
  std::strncpy(dst, src.c_str(), n - 1);
  dst[n - 1] = '\0';
}

Invoice compute_invoice(const Account &a, const UsageRec &u) {
  tr_account ra;
  tr_invoice ri;
  Invoice inv;

  copy_field(ra.province, sizeof(ra.province), a.province);
  ra.plan_fee = a.plan_fee;
  ra.included_gb = a.included_gb;
  ra.prev_plan_fee = a.prev_plan_fee;
  ra.plan_change_day = a.plan_chg_day;
  ra.line_count = a.line_cnt;
  ra.promo_amount = a.promo_amt;
  copy_field(ra.promo_issued_on, sizeof(ra.promo_issued_on), a.promo_dt);
  ra.suspension_start_day = a.susp_start;
  ra.suspension_end_day = a.susp_end;
  ra.prior_balance = a.prior_bal;
  copy_field(ra.prior_due_date, sizeof(ra.prior_due_date), a.prior_due);
  ra.loyalty_pct = a.loyalty_pct;

  tr_compute_invoice(&ra, u.usage_mb, u.period.c_str(), &ri);

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
  inv.loyalty_amt = ri.loyalty_discount;
  inv.federal_tax_amt = ri.federal_tax;
  inv.provincial_tax_amt = ri.provincial_tax;
  inv.federal_label = ri.federal_label;
  inv.provincial_label = ri.provincial_label;
  inv.total = ri.total;
  return inv;
}
