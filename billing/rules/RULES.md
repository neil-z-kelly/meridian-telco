# Shared billing rules

`telco_rules.c` is the single implementation of every billing rule meridian and
vantage both apply. There is no second implementation in any other language.

| Consumer | How it gets the rules |
| --- | --- |
| meridian `billing/` (C++) | links `telco_rules.o` directly; `billing/invoice.cpp` only marshals fields |
| vantage `app/billing` (Python) | vendors this source verbatim under `vendor/telco_rules/`, builds it and calls `libtelcorules.so` through `ctypes` |
| vantage `java/vantage-report` (Java) | cannot link or dlopen it, so it is held to `conformance/vectors.json`, which is generated from this source by `make vectors` |

Rule ids (`R-RATE`, `R-PRORATE`, …) are used by the header comments, the
conformance vectors and the java coverage map, so a rule can be traced from the
C function to the test that pins it.

## Where the two systems disagreed

Every row below produced different money for the same customer and the same
usage. The shared library implements the legacy (meridian) reading in all of
them, so vantage totals move onto meridian's.

| Rule | Legacy meridian — now shared | vantage before | Effect |
| --- | --- | --- | --- |
| `R-RATE` | whole gigabytes, partial gigabytes round up, $10.00/GB over the allowance | exact megabytes at $0.012/MB ($12.288/GB) | largest single source of variance |
| `R-PRORATE` | 30-day billing month whatever the calendar says | actual calendar days in the period | every mid-cycle plan change |
| `R-PROMO` | a credit belongs to the cycle it was issued in and does not roll forward | live for 30 days from issue | credits issued late in a month |
| `R-SUSPEND` | a suspended line keeps its provisioning, month billed in full, no credit | suspended days credited at the daily rate | every suspension |
| `R-TAXBASE` | GST/HST on the pre-discount subtotal, PST/QST on the subtotal **after** the loyalty credit | both components on the pre-discount subtotal | every BC and QC account with loyalty |
| `R-ROUND` | each charge line is rounded to cents as it is produced, the total is the sum of the rounded lines | full-precision `Decimal` throughout, rounded once at the total | cent-level drift on most invoices |
| `R-LOYALTY` | credit is a percentage of the subtotal, applied after tax | same in Python; the java report module took the percentage of subtotal **plus tax** | java report only |

Rules that already agreed and are now shared anyway, so they cannot drift
apart later: `R-LINES` (3–9 lines 5%, 10+ 10%, off the recurring charge),
`R-LATEFEE` (10 days grace after the due date, then 1.5% of the balance
outstanding when the cycle opens), the province rate table, and the invoice
assembly order in `tr_compute_invoice`.

One behaviour was kept as-is deliberately rather than being called a
divergence: province codes are matched exactly, as meridian has always matched
them, where vantage upper-cased the code first. No account in either system
carries a lower-case province, so no invoice changes.

## Flagged: genuinely ambiguous, not decided here

These are not implemented in the library because the legacy behaviour does not
answer them. They need a finance ruling; each one is listed so it is not
silently resolved by whoever touches this next.

**A1 — what the java report module's flat `taxPct` means.**
`java/vantage-report` carries one `taxPct` per account (6.25) and no province,
while the rule is two components with two different bases (`R-TAXBASE`). A flat
rate can be read as the federal component only, or as a blended federal +
provincial rate — and a blended rate cannot reproduce the split bases at all
once a loyalty credit exists. Until that is ruled on, `R-TAXBASE` is marked
`FLAGGED` in the java coverage map and the module's tax figures are not held to
the vectors.

**A2 — where the gigabyte round-up happens when a customer has several usage
records in a period.** Both invoice paths rate one usage record at a time, so
two records of 1.5 GB each are billed as 2 GB + 2 GB. The java pipeline sums
usage per account first, which would bill the same customer 3 GB. Rounding up
per record and rounding up per account-period are both defensible readings of
"partial gigabytes round up" and they differ in the customer's favour or the
company's depending on the shape of the feed. The library rates whatever usage
figure it is handed and does not decide; the choice sits with the caller and is
flagged for the same ruling.

**A3 — rounding a negative line.** `tr_money` is the legacy
`floor(x * 100 + 0.5) / 100`, which on an exact half-cent tie rounds toward
positive infinity: `-0.005` becomes `-0.00`, where vantage's `ROUND_HALF_UP`
made it `-0.01`. Meridian has never produced a negative charge line (the only
credits it raises are subtracted from the subtotal, never rounded on their own),
so there is no legacy precedent to default to. The library keeps the legacy
formula unchanged; if credit lines are ever put on the register in their own
right, this needs deciding first.

## Changing a rule

1. Change `telco_rules.c` (and `telco_rules.h` if the signature moves).
2. `make check` in meridian — rebuilds, runs `bin/billing-test` and fails if the
   committed vectors are stale.
3. `make vectors` and commit `conformance/vectors.json`.
4. Re-vendor into vantage: `make -C ../vantage-telco vendor-sync` (or copy the
   files and refresh `vendor/telco_rules/MANIFEST.sha256`). Vantage's build fails
   while its vendored copy differs from this directory.
