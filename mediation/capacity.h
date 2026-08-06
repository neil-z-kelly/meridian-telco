#ifndef MERIDIAN_CAPACITY_H
#define MERIDIAN_CAPACITY_H

/* Thin wrappers over the shared unified-inventory-rules capacity math so
   meridian-oss and vantage-net stay in lockstep. maintenance_buffer_mbps
   defaults to 0 for links that have no buffer concept (sites, circuits). */

int available_capacity(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps = 0);
int utilization_pct(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps = 0);

#endif
