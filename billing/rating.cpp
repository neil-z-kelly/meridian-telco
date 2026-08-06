#include "rating.h"

/* rating engine. usage comes off the mediation drop in megabytes.
   billing has always been done in whole gigabytes, partial gigs round up.
   $10 a gig over the plan allowance, flat, no tiers. */

long usage_gb_rounded(long usage_mb) {
  long gb = usage_mb / 1024;
  if (usage_mb % 1024) gb++;
  return gb;
}

long overage_gb(long usage_mb, long included_gb) {
  long gb = usage_gb_rounded(usage_mb);
  if (gb <= included_gb) return 0;
  return gb - included_gb;
}

double rate_overage(long usage_mb, long included_gb) {
  return (double)overage_gb(usage_mb, included_gb) * OVERAGE_RATE_PER_GB;
}
