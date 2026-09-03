#include <stdio.h>

#include "telco_rules.h"

/* conformance vectors. writes rules/conformance-vectors.json, the answers the
   shared rules give for a fixed set of inputs.

   the java report module in vantage-telco cannot link this library on java 11,
   so it is held to these vectors instead: the vectors are generated here, from
   the one implementation, and asserted in RulesConformanceTest.

   regenerate with `make vectors`. */

static void overage(long usage_mb, long included_gb, int last) {
  printf("    {\"usage_mb\": %ld, \"included_gb\": %ld, \"overage_gb\": %ld, \"charge\": %.2f}%s\n",
         usage_mb, included_gb, tr_overage_gb(usage_mb, included_gb),
         tr_rate_overage(usage_mb, included_gb), last ? "" : ",");
}

static void rounding(double amount, int last) {
  printf("    {\"amount\": %.6f, \"rounded\": %.2f}%s\n", amount, tr_money(amount),
         last ? "" : ",");
}

int main(void) {
  printf("{\n");
  printf("  \"_generated_by\": \"make vectors in meridian-telco (rules/gen_vectors.c)\",\n");
  printf("  \"overage_rate_per_gb\": %.2f,\n", TR_OVERAGE_RATE_PER_GB);
  printf("  \"mb_per_gb\": %ld,\n", TR_MB_PER_GB);
  printf("  \"overage\": [\n");
  overage(0, 1000, 0);
  overage(1024L * 1000L, 1000, 0);
  overage(1024L * 1000L + 1L, 1000, 0);
  overage(1024L * 1000L + 1000L, 1000, 0);
  overage(1024L * 1023L + 512L, 1000, 0);
  overage(7472103L, 5000, 0);
  overage(3000000L, 2000, 1);
  printf("  ],\n");
  printf("  \"rounding\": [\n");
  rounding(0.005, 0);
  rounding(0.015, 0);
  rounding(12.344, 0);
  rounding(12.345, 0);
  rounding(690.625, 1);
  printf("  ]\n");
  printf("}\n");
  return 0;
}
