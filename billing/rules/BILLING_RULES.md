# Shared billing rules specification

This document plus `billing_rules.json` beside it is the **single source of
truth** for the invoice rules that both billing engines implement:

| binding | location | consumer |
| --- | --- | --- |
| C++ module | `meridian-telco/billing/rules.h`, `rules.cpp` | `billing/invoice.cpp` (register, invoice-api, billing-test) |
| Python package | `vantage-telco/app/billing/rules/` | `app/billing/invoices.py` (API, dashboard, parity) |

Both bindings must encode exactly the constants and formulas below. The spec
files are committed byte-identical to both repos (`meridian-telco/billing/rules/`
and `vantage-telco/app/billing/rules/`); each test suite asserts its binding's
constants against the JSON, and `vantage-telco/tools/parity` fails if the two
copies differ. Change the spec first, then both bindings, in lock-step.

Every **RESOLVED** rule below defaults to the legacy meridian-telco output.
Rules marked **RULE_AMBIGUOUS** are also implemented with the legacy default so
the two engines agree today, but the default has *not* been ratified — see
"Ambiguous rules awaiting decision".

## Conventions

* Amounts are dollars; `money(x)` is `floor(x * 100 + 0.5) / 100` evaluated
  in binary64 (see R1 for why this is not exactly half-up).
* `period` is `YYYY-MM`; the cycle opens on day 1 of that month.
* Day-of-month inputs (`change_day`, `susp_start`, `susp_end`) are 1-based;
  `0` means "not applicable".

## Constants

| name | value |
| --- | --- |
| `BILLING_MONTH_DAYS` | 30 |
| `MB_PER_GB` | 1024 |
| `OVERAGE_RATE_PER_GB` | 10.00 |
| `MULTI_LINE_TIER1_MIN_LINES` / `_PCT` | 3 / 5% |
| `MULTI_LINE_TIER2_MIN_LINES` / `_PCT` | 10 / 10% |
| `LATE_FEE_GRACE_DAYS` | 10 |
| `LATE_FEE_PCT` | 1.5% |
| `PROVINCE_RATES` | BC 5/7 (GST/PST), AB 5/0 (GST), ON 13/0 (HST), QC 5/9.975 (GST/QST); unknown province -> 5/0 (GST) |

## Rules

### R1 ROUNDING — RESOLVED (legacy)
Every charge line is rounded with `money()` as it is produced; the invoice
total is `money()` of the sum of the rounded lines.

Arithmetic model: each rule evaluates its formula in IEEE-754 binary64 in the
operation order written in this document, then applies
`floor(x * 100 + 0.5) / 100`. The Python binding mirrors this (float inside a
rule, `Decimal` at the boundary) rather than using exact `Decimal` half-up,
because the two differ at exact half-cent products: e.g. `1102.50 * 13 / 100`
is `143.325` in decimal but `143.32499…` in binary64, so legacy yields
`143.32` where true half-up yields `143.33`. This is a mechanical legacy quirk
kept for parity, not a ratified rounding policy; switching both bindings to
exact decimal half-up is a spec change (it moves a small number of invoices by
one cent).
*Was:* vantage carried `Decimal` at full precision and rounded once at the total.

### R2 PRORATION — RESOLVED (legacy)
```
daily_rate(fee)                      = fee / BILLING_MONTH_DAYS
prorated_plan_charge(fee, prev, day) =
    day <= 0        -> money(fee)
    days_old        = clamp(day - 1, 0, 30)
    days_new        = 30 - days_old
    money( money(daily_rate(prev) * days_old) + money(daily_rate(fee) * days_new) )
```
Calendar length of the month is ignored; February and July prorate alike.
*Was:* vantage used `monthrange` actual days, unrounded.

### R3 OVERAGE_RATING — RESOLVED (legacy)
```
usage_gb_rounded(mb) = ceil(mb / 1024)
overage_gb(mb, inc)  = max(usage_gb_rounded(mb) - inc, 0)
rate_overage(mb,inc) = overage_gb * OVERAGE_RATE_PER_GB
```
*Was:* vantage billed exact MB at $0.012/MB.

### R4 MULTI_LINE_DISCOUNT — RESOLVED (agrees)
`pct = 10 if lines >= 10 else 5 if lines >= 3 else 0`;
`multi_line_discount = money(plan_charge * pct / 100)`.

### R5 LATE_FEE — RESOLVED (agrees)
`days_past_due = max(period_start - due_date, 0)`;
fee is `0` if `prior_balance <= 0` or `days_past_due <= 10`, else
`money(prior_balance * 1.5 / 100)`.

### R6 LOYALTY_DISCOUNT — RESOLVED (agrees)
`loyalty = money(subtotal * loyalty_pct / 100)`; subtracted from the total
after tax; it does **not** reduce the federal tax base.

### R7 FEDERAL_TAX — RESOLVED (agrees)
`federal_tax = money(subtotal * federal_pct / 100)` on the pre-discount subtotal.

### R8 PROVINCIAL_TAX_BASE — RULE_AMBIGUOUS (legacy default)
Default (legacy): `provincial_tax = money(max(subtotal - loyalty, 0) * provincial_pct / 100)`.
Alternative (vantage): base is the pre-discount `subtotal`.
Zero when `provincial_pct == 0`.

### R9 PROMO_EXPIRY — RULE_AMBIGUOUS (legacy default)
Default (legacy): `promo_is_live(issued_on, period)` iff `issued_on` falls in
the same calendar year+month as `period`. `promo_credit = money(amount)` when
live and `amount > 0`, else `0`.
Alternative (vantage): live iff `issued_on + 30 days >= period_start`.

### R10 SUSPENSION_CREDIT — RULE_AMBIGUOUS (legacy default)
`suspended_days(start, end) = end - start + 1` when `start > 0 && end >= start`, else `0`.
Default (legacy): `suspension_credit = 0` always.
Alternative (vantage): `daily_rate(fee) * suspended_days`.

## Invoice assembly (both engines)

```
plan_charge   = prorated_plan_charge(plan_fee, prev_plan_fee, plan_chg_day)
line_discount = multi_line_discount(plan_charge, line_cnt)
recurring     = money(plan_charge - line_discount)
overage       = money(rate_overage(usage_mb, included_gb))
susp_credit   = suspension_credit(plan_fee, susp_start, susp_end)
promo         = promo_credit(promo_amt, promo_dt, period)
late          = late_fee(prior_bal, prior_due, period)
subtotal      = money(max(recurring + overage + late - susp_credit - promo, 0))
rates         = rates_for_province(province)
loyalty       = loyalty_discount(subtotal, loyalty_pct)
federal       = federal_tax(subtotal, rates)
provincial    = provincial_tax(subtotal, loyalty, rates)
total         = money(subtotal - loyalty + federal + provincial)
```

## Ambiguous rules awaiting decision

These are implemented with the legacy default **only so the two engines stop
drifting**. Nobody has ruled that the legacy behaviour is correct, and in each
case it may be legally or contractually wrong. Each binding exposes the flags
(`RULE_AMBIGUOUS_*` in C++, `AMBIGUOUS_RULES` in Python) so downstream tooling
can surface them.

| rule | legacy default | alternative | why it matters | who decides |
| --- | --- | --- | --- | --- |
| R8 PROVINCIAL_TAX_BASE | PST/QST on subtotal minus loyalty discount | PST/QST on pre-discount subtotal | Whether a post-invoice goodwill credit reduces the provincial tax base is a tax-authority question (BC PST, Revenu Québec QST). The legacy treatment differs from how the same engines treat GST/HST and may under-remit. | Finance / tax compliance |
| R9 PROMO_EXPIRY | expires at end of issue cycle | live 30 days from issue | Depends on the wording of the promo terms shown to customers; expiring early may breach the offer. | Marketing / legal |
| R10 SUSPENSION_CREDIT | full month billed, no credit | suspended days credited at daily rate | Charging for days with no service is a customer-contract / consumer-protection question (e.g. CRTC Wireless Code). | Legal / customer contracts |

Until a decision lands, do not change these defaults in one binding without
changing the spec and the other binding.
