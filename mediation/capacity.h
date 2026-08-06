#ifndef MERIDIAN_CAPACITY_H
#define MERIDIAN_CAPACITY_H

int available_capacity(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps = 0);
int utilization_pct(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps = 0);

#endif
