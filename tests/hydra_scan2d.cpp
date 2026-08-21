/**
 * GYR-SIEVE-002 Ticket C Hydra record — Scan2D NOT SHIPPED
 *
 * Family attempted: blocked prefix-max (sqrt-decomposition on y-ranks).
 * Distinct from Fenwick tree; equal-x batch; full layers; bit-identical on identity suite.
 * Not an SFS / active-front retry (that family failed earlier at ~37× on N=1e5).
 *
 * H-DOM N=1e6 (~1% rank-1), best-of-3 wall:
 *   Fenwick:  ~530 ms
 *   Scan2D:  ~1193 ms
 *   ratio:    ~2.25  (need ≤ 0.70)
 *
 * H-MEM: not claimed.
 * Outcome (GYR-SIEVE-002 §4): delete Scan2D. No Strategy::Scan2D. No kernel in header.
 * Successful Ticket C (honest deletion with measured numbers at N=1e6).
 *
 * This file is a decision record only; main() returns 0. No 2.25× kernel on the default path.
 */
int main() { return 0; }
