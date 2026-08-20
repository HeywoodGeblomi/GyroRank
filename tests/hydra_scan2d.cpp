/**
 * GYR-SIEVE-001 Track 0 Hydra record for Scan2D
 *
 * Attempted family: active-front / layered sweep (SFS-style window).
 * Identity vs Fenwick: PASS on the full existing fixture suite (bit-identical).
 * H-DOM (N=1e5 proxy of 1e6 class, ~1% rank-1): Scan wall ≈ 37× Fenwick.
 * H-DOM win condition (≤ 0.70×) NOT met.
 * H-MEM: not claimed.
 *
 * Outcome (GYR-SIEVE-001 §2.2): delete Scan2D from the public surface.
 * No Strategy::Scan2D. No U[k] entry. Kernel not shipped.
 * This is a successful Track 0 (honest deletion after Hydra loss).
 *
 * Decision: Track 0 = Scan2D NOT SHIPPED (Hydra fail). v0.2 Fenwick-only remains.
 */
int main() {
    // No kernel to measure. Exit 0 documents the closed decision.
    return 0;
}
