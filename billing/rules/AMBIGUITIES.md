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
Java report used to sum every row it saw for an account. The report no longer
rates anything, so today the question is only about the two rating callers:
with one row per account per period in the fixtures they agree by accident. If
a second row can ever arrive the answer is either "sum the megabytes onto one
invoice" or "one invoice per row", and it changes the plan charge, not just the
overage.

## 3. Archived reports rendered at the old flat rate

The Java report used to compute tax itself, at a single flat 6.25% with no
province at all, so every artifact the NOC has archived quotes that rate. The
module now prints the tax lines the rules assessed, which means new artifacts
for the same cycle will not match the filed ones. Nobody has said whether past
cycles should be re-rendered, or left as the record of what was invoiced at the
time.

## 4. Rounding an amount below zero

`br_money` is `floor(x * 100 + 0.5) / 100`, which sends -0.005 to -0.00, while
vantage's `Decimal(ROUND_HALF_UP)` sends it to -0.01. No rule currently
produces a negative amount: credits are positive and subtracted and the
subtotal is clamped at zero. Should a future credit be able to push a line
negative, the two halves of this library will part company at the half cent
until the direction is chosen deliberately.
