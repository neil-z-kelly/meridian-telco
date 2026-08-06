#include "circuit_counter.h"

/* circuit counts for the nightly inventory roll-up */

int count_active_circuits(const std::vector<Row> &circuits) {
  int n = 0;
  for (size_t i = 0; i < circuits.size(); i++) {
    Row r = circuits[i];
    if (to_int(r["STATUS_CD"]) != 1) continue;
    /* role does not matter here, a standby circuit is still a circuit we own */
    n++;
  }
  return n;
}

int sum_active_capacity(const std::vector<Row> &circuits) {
  int total = 0;
  for (size_t i = 0; i < circuits.size(); i++) {
    Row r = circuits[i];
    if (to_int(r["STATUS_CD"]) != 1) continue;
    total += to_int(r["CAP_MBPS"]);
  }
  return total;
}
