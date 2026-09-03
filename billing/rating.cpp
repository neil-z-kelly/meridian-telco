#include "rating.h"

/* rating engine. usage comes off the mediation drop in megabytes.
   billing has always been done in whole gigabytes, partial gigs round up.
   $10 a gig over the plan allowance, flat, no tiers.
   rule R3 in billing/rules/BILLING_RULES.md */

long usage_gb_rounded(long usage_mb) {
  return rules::usage_gb_rounded(usage_mb);
}

long overage_gb(long usage_mb, long included_gb) {
  return rules::overage_gb(usage_mb, included_gb);
}

double rate_overage(long usage_mb, long included_gb) {
  return rules::rate_overage(usage_mb, included_gb);
}
