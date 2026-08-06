#include "status.h"

/* status codes, per the 2009 spec doc (see ops wiki, page is gone)
   1 = ACTIVE
   2 = PLANNED
   3 = DECOMMISSIONED
   field techs also use 3 when a site is down for a while, was supposed to be
   a temporary thing until we added a fourth code. never happened. */

std::string status_label(int status_cd) {
  switch (status_cd) {
    case STATUS_ACTIVE: return "ACTIVE";
    case STATUS_PLANNED: return "PLANNED";
    case STATUS_DECOMM: return "DECOMMISSIONED";
  }
  return "UNKNOWN";
}

bool status_is_countable(int status_cd) {
  return status_cd == STATUS_ACTIVE || status_cd == STATUS_PLANNED;
}
