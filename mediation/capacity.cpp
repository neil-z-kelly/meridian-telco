#include "capacity.h"
#include "unified_inventory_rules/capacity.h"

/* Delegate to the shared canonical rule (unified-inventory-rules). */

int available_capacity(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps) {
  return unified_inventory_rules::available_capacity(total_mbps, allocated_mbps,
                                                     maintenance_buffer_mbps);
}

int utilization_pct(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps) {
  double pct = unified_inventory_rules::utilization_pct(total_mbps, allocated_mbps,
                                                        maintenance_buffer_mbps);
  return (int)(pct + 0.5);
}
