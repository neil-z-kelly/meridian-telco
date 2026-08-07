#ifndef OSS_CAPACITY_H
#define OSS_CAPACITY_H

/* Canonical capacity math shared by meridian-oss and vantage-net.

   available_capacity = max(total - allocated - buffer, 0)
   utilization_pct    = (allocated + buffer) * 100 / total, 0 when total <= 0

   The maintenance buffer defaults to 0, so existing two-argument call sites
   keep compiling and keep their current numbers. */

int available_capacity(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps = 0);
int utilization_pct(int total_mbps, int allocated_mbps, int maintenance_buffer_mbps = 0);

#endif
