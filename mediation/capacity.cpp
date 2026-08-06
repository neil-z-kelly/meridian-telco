#include "capacity.h"

/* capacity math. the maintenance buffer is withheld on top of what is
   allocated: capacity we need during a maintenance window is not sellable. */

int available_capacity(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps) {
  int avail = total_mbps - allocated_mbps - maintenance_buffer_mbps;
  if (avail < 0) avail = 0;
  return avail;
}

int utilization_pct(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps) {
  if (total_mbps <= 0) return 0;
  return ((allocated_mbps + maintenance_buffer_mbps) * 100) / total_mbps;
}
