# Unresolved rules

Everywhere meridian and vantage disagreed, the shared library keeps meridian's
legacy behaviour, because that is what has been invoiced. The cases below are
different: the two engines disagree *and* there is no legacy answer to fall
back on, because no customer has ever exercised the case. They are implemented
one way so the code runs, but the choice is a guess and needs a decision from
billing before anyone relies on it.

## 1. Province codes outside the table

`br_rates_for_province` matches `BC`, `AB`, `ON` and `QC` exactly and silently
falls back to 5% GST with no provincial component. Vantage upper-cased the code
first, so `bc` was taxed as British Columbia there and as the fallback here.
Every account in both books is one of the four codes in the table, so nothing
says whether an unknown or oddly cased province should be

  - taxed federally only (what both engines happen to do today, and what the
    library does), or
  - rejected as bad account data before it reaches an invoice.

Silently under-taxing a real account is the worse of the two failure modes, so
this one wants an answer.

## 2. Several usage records for one account and period

The library rates one usage figure per invoice; who aggregates is the caller's
business, and the callers do not agree. `billing-run` and vantage's python
emit one invoice per usage row, each carrying a full plan charge, while the
Java report sums every row it sees for an account. With one row per account
per period in the fixtures the three agree by accident. If a second row can
ever arrive the answer is either "sum the megabytes onto one invoice" or "one
invoice per row", and it changes the plan charge, not just the overage.

## 3. Tax in the Java report module

`RatingEngine` used a single flat 6.25% rate with no province at all. The
report is now checked against the shared vectors for the rules it can compute,
but its tax figures cannot be reconciled until someone decides whether the
report should use the province table (its seed data carries no province, so
this is a data question) or keep quoting a blended rate.

## 4. Rounding an amount below zero

`br_money` is `floor(x * 100 + 0.5) / 100`, which sends -0.005 to -0.00, while
vantage's `Decimal(ROUND_HALF_UP)` sends it to -0.01. No rule currently
produces a negative amount: credits are positive and subtracted and the
subtotal is clamped at zero. Should a future credit be able to push a line
negative, the two halves of this library will part company at the half cent
until the direction is chosen deliberately.
