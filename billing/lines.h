#ifndef MERIDIAN_LINES_H
#define MERIDIAN_LINES_H

/* multi line discount off the recurring charge. 3 to 9 lines 5%, 10 or more
   10%. same schedule finance publishes in the rate card. */

double multi_line_pct(int line_count);
double multi_line_discount(double recurring_charge, int line_count);

#endif
