#ifndef MERIDIAN_DISCOUNTS_H
#define MERIDIAN_DISCOUNTS_H

double apply_loyalty(double amount, double loyalty_pct);
double apply_tax(double amount, double tax_pct);
double invoice_total(double charges, double loyalty_pct, double tax_pct);

#endif
