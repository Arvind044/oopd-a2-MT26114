# OOPD Assignment 2 — Answers

**Student:** Arvind Bhagat  
**Roll Number:** MT26114  
**Course:** OOPD  
**Assignment:** 2 — Classes, Objects, Inheritance and Exception Handling

## Questions Attempted

Q1–Q16

## Written Answers and Design Decisions

### Q1 — Journal Resource
Added `Journal` as a concrete `Resource` type with ISSN, issues per year, and subscription years. Its cost is based on unit price, copies, issues/subscription information as required by the assignment.

### Q2 — EBook Resource
Added `EBook` as a concrete `ElectronicResource` with authors, ISBN, format, and DRM information.

The EBook reuses the electronic-resource hierarchy because it is a digital resource. Some metadata overlaps with `Book`; this could be further reduced in a larger design by introducing a reusable metadata/base component, but the current hierarchy preserves the existing public interfaces and assignment structure.

### Q3 — AudioBook and Thesis
`AudioBook` derives from `ElectronicResource` because it is a digital resource and therefore shares the licensed-seat/electronic-resource behaviour.

`Thesis` derives from `Resource` because it is treated as a non-digital/print resource. It stores university, degree, and supervisor information and has no acquisition cost as required.

### Q4 — Hardcover Pricing
Hardcover books receive the required 20% price increase over the base book price.

### Q5 — Bulk Pricing
Bulk pricing applies the required discount when the number of purchased copies reaches the assignment threshold. The pricing is calculated before the applicable tax.

### Q6 — Taxes
Added resource-category tax rates and included pre-tax and post-tax costs in purchase records. Budget, quota, and spending checks use the post-tax purchase amount.

### Q7 — Title Quotas
Implemented category-level title quotas. A quota of zero means unlimited titles. Repeated purchases of an existing title are allowed, while introducing a new title beyond the quota is rejected without changing budget or holdings state.

### Q8 — Cancellation
Added purchase cancellation with validation for invalid order numbers, non-approved orders, and already-cancelled orders. Cancellation reverses the appropriate budget/holding effects and records a cancellation entry.

The title remains known after cancellation so that cancelling one purchase cannot incorrectly remove a title that may have been purchased multiple times.

### Q9 — Department Budgets
Added department-specific budgets and associated department information to purchase requests and records. Department limits are enforced in addition to the global budget and category quotas.

### Q10 — Budget Rollover
Implemented percentage-based rollover of the remaining budget. The carried-forward amount can be combined with an optional base budget, while category quotas are preserved and usage counters are reset.

### Q11 — All-or-Nothing Batch Processing
Added `processBatch(reqs, allOrNothing)`.

For all-or-nothing processing, the complete batch is first simulated using cumulative budget, category quota, title quota, and department-budget state. If any request fails, no purchase is committed. If all requests pass, the real purchases are then performed.

### Q12 — Multiple Vendors
Added support for multiple vendors per resource/title. The system selects the vendor with the lowest post-tax total cost. If costs tie, the lexicographically smaller vendor name is selected.

Existing pricing rules such as hardcover pricing, bulk discounts, electronic-resource seat pricing, and journal subscriptions are preserved.

### Q13 — Catalog Search and Persistence
Added catalog searches by author, ISBN/ISSN, and year range.

Catalog persistence uses explicit type tags so that the correct concrete resource type is reconstructed when loading. Holdings are preserved. Loading is atomic: malformed input or an unknown type does not partially replace the existing catalog.

### Q14 — Lending and Electronic Sessions
Added `LendingManager` for:
- borrowing and returning print resources;
- opening and closing electronic-resource sessions.

Available print copies are calculated as:

`holdings - active loans`

Available electronic seats are calculated as:

`holdings - active sessions`

Lending/session operations do not decrement catalog holdings because holdings represent owned/licensed capacity, not currently borrowed/active units.

Duplicate borrow/session operations and capacity violations are rejected with the appropriate exceptions.

### Q15 — Currency Support
Extended `Money` with a currency code, defaulting to INR.

Addition, subtraction, and comparisons between different currencies throw `CurrencyMismatchError`. Scalar multiplication preserves the original currency.

### Q16 — Test Code Cleanliness
Replaced the custom test macros with standard C++ `assert` and an `assertThrows` helper. The test meanings and coverage were preserved while removing the macro-based checks.

## Verification

- All assignment tests pass.
- Final test output: `All tests passed!`
- Project builds successfully with the MSYS2 UCRT64 GCC toolchain.
- `git diff --check` is clean.
- The Git repository is included as required by the assignment.
