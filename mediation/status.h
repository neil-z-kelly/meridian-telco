#ifndef MERIDIAN_STATUS_H
#define MERIDIAN_STATUS_H

#include <string>

#define STATUS_ACTIVE 1
#define STATUS_PLANNED 2
#define STATUS_DECOMM 3

std::string status_label(int status_cd);
bool status_is_countable(int status_cd);

#endif
