#ifndef MERIDIAN_CIRCUIT_COUNTER_H
#define MERIDIAN_CIRCUIT_COUNTER_H

#include <string>
#include <vector>
#include "csv.h"

#define ROLE_PRIMARY 1
#define ROLE_STANDBY 2
#define ROLE_FAILOVER 3

int count_active_circuits(const std::vector<Row> &circuits);
int sum_active_capacity(const std::vector<Row> &circuits);

#endif
