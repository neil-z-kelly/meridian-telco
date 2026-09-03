#ifndef MERIDIAN_RATING_H
#define MERIDIAN_RATING_H

#include "../rules/telco_rules.h"

/* whole gigabytes, partial gigs round up, $10 a gig over plan.
   implemented once in rules/telco_rules.c. */

#define OVERAGE_RATE_PER_GB TR_OVERAGE_RATE_PER_GB

inline long usage_gb_rounded(long usage_mb) { return tr_usage_gb_rounded(usage_mb); }
inline long overage_gb(long usage_mb, long included_gb) {
  return tr_overage_gb(usage_mb, included_gb);
}
inline double rate_overage(long usage_mb, long included_gb) {
  return tr_rate_overage(usage_mb, included_gb);
}

#endif
