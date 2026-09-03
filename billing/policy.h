#ifndef MERIDIAN_POLICY_H
#define MERIDIAN_POLICY_H

#include "../rules/telco_rules.h"

/* meridian billing policy. every rule lives in rules/telco_rules.c; this is the
   one place where meridian says which side of an unresolved rule it runs on.

   provincial tax base: PST and QST are assessed on what the customer actually
   pays, so the loyalty credit comes off first. finance signed this off in 2010,
   do not change without a ticket. RS 2010-06. vantage-telco runs the other
   setting on purpose, see the parity report. */

inline TrPolicy meridian_policy() {
  TrPolicy p = tr_policy_legacy();
  p.provincial_tax_base = TR_TAX_BASE_POST_DISCOUNT;
  return p;
}

#endif
