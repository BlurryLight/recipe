"""
Floating-point precision demo
=============================

Visualizes, in an intuitive way, the *value range* and the *ULP (unit in the last
place)* behavior of common floating-point formats:

    double / float / half / bf16 / fp8 (e4m3, e5m2) / fp4 (e2m1)

For a given target value (e.g. 10000.0) it answers the two questions you usually
care about:
    1. Can this format represent it?            -- range / overflow / exact hit
    2. How precise is this format at that value? -- ULP (gap between adjacent
       representable values), rounding error, usable decimal digits.

Implementation notes:
    - Every format is encoded/decoded at the *bit level* (we assemble exponent and
      mantissa ourselves), so low-precision formats with no native support
      (fp8 / fp4) are simulated exactly.
    - Rounding is done with fractions.Fraction for *exact* round-half-to-even, so
      the precision demo is not polluted by host float error.
    - The matplotlib plots are an optional enhancement; if matplotlib is missing,
      the full text analysis still runs.

Dependencies: numpy, matplotlib -- create the env with python_examples/setup_venv.sh.
"""

from __future__ import annotations

import math
import os
from fractions import Fraction

# matplotlib / numpy are optional: if missing, skip plots, keep the text analysis.
try:
    import matplotlib.pyplot as plt
    import numpy as np
    _HAS_PLOT = True
except Exception:  # pragma: no cover
    _HAS_PLOT = False

# Focused x-window for the staircase / relative-precision plots. It contains every
# target value and the full range of every small format; double/float/bf16 extend
# far beyond it on the high end, but that part is just a parallel copy of the same
# shape, so clipping it changes nothing about the takeaway.
_PLOT_XMIN, _PLOT_XMAX = 1e-8, 1e9


# --------------------------------------------------------------------------- #
#  Basic helpers
# --------------------------------------------------------------------------- #
def _round_to_even(fr: Fraction) -> int:
    """Round a Fraction to the nearest integer; break exact ties to even."""
    n, d = fr.numerator, fr.denominator
    q, r = divmod(n, d)          # n = q*d + r,  0 <= r < d
    cmp = 2 * r - d              # compare 2r to d without floating point
    if cmp > 0:
        return q + 1
    if cmp < 0:
        return q
    return q if (q & 1) == 0 else q + 1   # tie -> even


def _pow2(shift: int) -> Fraction:
    """Exact 2**shift, shift may be negative."""
    return Fraction(1 << shift) if shift >= 0 else Fraction(1, 1 << -shift)


def fmt(x: float, prec: int = 4) -> str:
    """Uniform number formatting for aligned tables."""
    if x == 0:
        return "0"
    if math.isinf(x):
        return "+inf" if x > 0 else "-inf"
    if math.isnan(x):
        return "NaN"
    a = abs(x)
    if a < 1e-3 or a >= 1e6:
        return f"{x:.{prec}e}"
    return f"{x:.{prec}f}"


# --------------------------------------------------------------------------- #
#  FloatFormat: bit-level codec + precision analysis
# --------------------------------------------------------------------------- #
class FloatFormat:
    """Bit-level simulation of one IEEE-754 style floating format.

    Args:
        name       display name
        exp_bits   exponent field width
        mant_bits  mantissa field width (without the implicit leading 1)
        bias       exponent bias
        has_inf    True  = standard IEEE: all-ones exponent means +/-inf / NaN
                   (double / float / half / bf16 / e5m2)
                   False = no inf (e4m3 / e2m1)
        has_nan    whether a NaN encoding is reserved (e4m3 yes, e2m1 no)
    """

    def __init__(self, name: str, exp_bits: int, mant_bits: int, bias: int,
                 has_inf: bool = True, has_nan: bool = True):
        self.name = name
        self.exp_bits = exp_bits
        self.mant_bits = mant_bits
        self.bias = bias
        self.has_inf = has_inf
        self.has_nan = has_nan

        self.total_bits = 1 + exp_bits + mant_bits
        self.sig_bits = exp_bits + mant_bits          # width without the sign bit
        self.sign_bit = 1 << self.sig_bits
        self.mag_mask = (1 << self.sig_bits) - 1
        self.all_ones_exp = (1 << exp_bits) - 1

        # Stored exponent of the largest finite normal: with inf, all-ones is
        # reserved for inf/nan; without inf, all-ones is a valid exponent.
        self.max_exp_field = self.all_ones_exp - 1 if has_inf else self.all_ones_exp
        # Mantissa of the largest finite value: e4m3 is NaN at (all-ones exp,
        # all-ones mant), so the top finite mantissa is one less.
        self.max_mant_field = (1 << mant_bits) - 1
        if has_nan and not has_inf:
            self.max_mant_field = (1 << mant_bits) - 2
        self._max_finite_mag = (self.max_exp_field << mant_bits) | self.max_mant_field
        self._inf_mag = (self.all_ones_exp << mant_bits) if has_inf else None

    # ---------------- basic properties ----------------
    @property
    def max_finite(self) -> float:
        return self._decode_mag(self._max_finite_mag)

    @property
    def min_normal(self) -> float:
        return 2.0 ** (1 - self.bias)

    @property
    def min_subnormal(self) -> float:
        return 2.0 ** (1 - self.bias - self.mant_bits)

    @property
    def machine_epsilon(self) -> float:
        """Size of one ULP at 1.0 = 2**-mant_bits."""
        return 2.0 ** (-self.mant_bits)

    @property
    def decimal_digits(self) -> float:
        """Guaranteed decimal digits ~= mant_bits * log10(2)."""
        return self.mant_bits * math.log10(2)

    def count_positive_finite(self) -> int:
        """Number of positive finite representable values (excluding +0)."""
        sub = (1 << self.mant_bits) - 1                  # subnormals
        norm = 0
        for E in range(1, self.max_exp_field + 1):
            if E == self.all_ones_exp and self.has_nan and not self.has_inf:
                norm += (1 << self.mant_bits) - 1        # top all-ones mantissa is NaN
            else:
                norm += (1 << self.mant_bits)
        return sub + norm

    # ---------------- encode: float -> bits ----------------
    def encode(self, x: float) -> int:
        """Encode a Python float into the integer bit pattern of this format
        (round-to-nearest, ties-to-even)."""
        if x != x:                                       # NaN
            return self._nan_bits()
        if x == 0.0:
            return 0 if math.copysign(1.0, x) > 0 else self.sign_bit
        if math.isinf(x):
            return (self.sign_bit if x < 0 else 0) | (self._inf_mag or 0)
        sign_bit = self.sign_bit if x < 0 else 0
        return sign_bit | self._encode_mag(Fraction(abs(x)))

    def _encode_mag(self, f: Fraction) -> int:
        # Find e with 2**e <= f < 2**(e+1) (exact; bit_length is a starting guess).
        e = f.numerator.bit_length() - f.denominator.bit_length()
        while True:
            pow_e = _pow2(e)
            if pow_e > f:
                e -= 1
            elif pow_e * 2 <= f:
                e += 1
            else:
                break
        E_stored = e + self.bias

        if E_stored > self.max_exp_field:                # overflow -> saturate
            return self._max_finite_mag
        if E_stored <= 0:                                # falls in subnormal range
            return self._encode_subnormal(f)

        # Normal: quantize f by ULP = 2**(e - mant_bits)
        M = _round_to_even(f / _pow2(e - self.mant_bits))  # M in [2**mant, 2**(mant+1)]
        if M >= (1 << (self.mant_bits + 1)):             # rounding carried into next binade
            M >>= 1
            E_stored += 1
            if E_stored > self.max_exp_field:
                return self._max_finite_mag
        mant_field = M - (1 << self.mant_bits)
        return (E_stored << self.mant_bits) | mant_field

    def _encode_subnormal(self, f: Fraction) -> int:
        q = _pow2(1 - self.bias - self.mant_bits)        # subnormal quantum
        k = _round_to_even(f / q)
        if k == 0:
            return 0                                     # underflow to zero
        if k >= (1 << self.mant_bits):                   # rounded up to min normal
            return 1 << self.mant_bits                   # E_stored=1, mant=0
        return k                                         # subnormal, E_stored=0

    def _nan_bits(self) -> int:
        if self.has_nan:
            return (self.all_ones_exp << self.mant_bits) | (1 << max(0, self.mant_bits - 1))
        return self._max_finite_mag                      # no NaN encoding: return max finite

    # ---------------- decode: bits -> float ----------------
    def decode(self, bits: int) -> float:
        sign = (bits >> self.sig_bits) & 1
        exp_field = (bits >> self.mant_bits) & self.all_ones_exp
        mant_field = bits & ((1 << self.mant_bits) - 1)

        if self.has_inf and exp_field == self.all_ones_exp:
            if mant_field == 0:
                return math.copysign(math.inf, -1.0 if sign else 1.0)
            return math.nan
        if (not self.has_inf) and self.has_nan and exp_field == self.all_ones_exp \
                and mant_field == (1 << self.mant_bits) - 1:
            return math.nan

        val = self._decode_mag(bits & self.mag_mask)
        return -val if sign else val

    def _decode_mag(self, mag: int) -> float:
        exp_field = (mag >> self.mant_bits) & self.all_ones_exp
        mant_field = mag & ((1 << self.mant_bits) - 1)
        if exp_field == 0:
            if mant_field == 0:
                return 0.0
            return mant_field * 2.0 ** (1 - self.bias - self.mant_bits)
        sig = 1.0 + mant_field / (1 << self.mant_bits)
        return sig * 2.0 ** (exp_field - self.bias)

    # ---------------- precision-analysis API ----------------
    def round(self, x: float) -> float:
        """Round x to the nearest representable value of this format."""
        return self.decode(self.encode(x))

    def ulp_at(self, x: float) -> float:
        """ULP of the binade that x rounds into: the gap between two adjacent
        representable values at that magnitude. This is the core 'precision
        capability' metric -- a bigger ULP means nearby numbers cannot be told
        apart."""
        exp_field = (self.encode(x) >> self.mant_bits) & self.all_ones_exp
        if exp_field == 0:                               # subnormal: fixed spacing
            return 2.0 ** (1 - self.bias - self.mant_bits)
        return 2.0 ** (exp_field - self.bias - self.mant_bits)

    def next_up(self, x: float) -> float:
        """Next representable value greater than x (x is rounded first); at the
        top it returns max_finite or +inf."""
        return self.decode(self._next_up_bits(self.encode(x)))

    def _next_up_bits(self, bits: int) -> int:
        sign = bits >> self.sig_bits
        mag = bits & self.mag_mask
        if sign == 0:                                    # positive: grow magnitude
            if self._inf_mag is not None and mag == self._inf_mag:
                return bits
            if mag >= self._max_finite_mag:
                return self._inf_mag if self._inf_mag is not None else self._max_finite_mag
            return mag + 1
        if mag == 0:                                     # -0 -> smallest +subnormal
            return 1
        return self.sign_bit | (mag - 1)                 # negative: shrink magnitude

    def adjacent(self, x: float):
        """Three representable values bracketing x: (prev, rounded, next)."""
        r = self.round(x)
        return -self.next_up(-r), r, self.next_up(r)

    def analyze(self, x: float) -> dict:
        """Full precision analysis of a target value x."""
        r = self.round(x)
        ulp = self.ulp_at(x)
        err = abs(x - r)
        rel = err / abs(x) if x != 0 else 0.0
        return {
            "x": x,
            "in_range": abs(x) <= self.max_finite,
            "rounded": r,
            "ulp": ulp,
            "abs_err": err,
            "rel_err": rel,
            "digits": (-math.log10(rel) if 0 < rel < 1 else math.inf),
            "neighbors": self.adjacent(x),
        }


# --------------------------------------------------------------------------- #
#  All formats to demonstrate
# --------------------------------------------------------------------------- #
FORMATS = [
    FloatFormat("double fp64",  11, 52, 1023),
    FloatFormat("float  fp32",   8, 23,  127),
    FloatFormat("half   fp16",   5, 10,   15),
    FloatFormat("bf16   e8m7",   8,  7,  127),
    FloatFormat("fp8    e5m2",   5,  2,   15),
    FloatFormat("fp8    e4m3",   4,  3,    7, has_inf=False, has_nan=True),
    FloatFormat("fp4    e2m1",   2,  1,    1, has_inf=False, has_nan=False),
]

# Target values to analyze in detail: small, unit, large.
TARGETS = [0.001, 1.0, 10000.0]


# --------------------------------------------------------------------------- #
#  Text output
# --------------------------------------------------------------------------- #
def print_properties_table(formats):
    print("=" * 96)
    print("Format overview")
    print("=" * 96)
    header = (f"{'format':<14}{'bit':>4}{'max finite':>14}{'min normal':>13}"
              f"{'min subnorm':>13}{'ULP@1':>12}{'dec digits':>11}")
    print(header)
    print("-" * 96)
    for f in formats:
        print(f"{f.name:<14}"
              f"{f.total_bits:>4}"
              f"{fmt(f.max_finite, 2):>14}"
              f"{fmt(f.min_normal, 2):>13}"
              f"{fmt(f.min_subnormal, 2):>13}"
              f"{fmt(f.machine_epsilon, 3):>12}"
              f"{f.decimal_digits:>11.2f}")
    print("\nULP@1 = 2**-mant_bits, the order of magnitude of machine epsilon; "
          "'dec digits' ~= mant_bits * log10(2).\n")


def print_target_analysis(formats, x):
    print("=" * 96)
    print(f"Target value x = {x!r}")
    print("=" * 96)
    header = (f"{'format':<14}{'represent?':>11}{'rounded':>14}{'ULP (gap)':>13}"
              f"{'abs error':>13}{'rel error':>11}{'digits':>8}  neighbors [prev, next]")
    print(header)
    print("-" * 96)
    for f in formats:
        a = f.analyze(x)
        if not a["in_range"]:
            note = f"out of range, max is {fmt(f.max_finite)}"
            print(f"{f.name:<14}{'overflow':>11}"
                  f"{fmt(a['rounded']):>14}{fmt(a['ulp']):>13}"
                  f"{'--':>13}{'--':>11}{'--':>8}  {note}")
            continue

        tag = "exact" if a["abs_err"] == 0 else "approx"
        digs = f"{a['digits']:.2f}" if math.isfinite(a["digits"]) else "  inf"
        lo, _, hi = a["neighbors"]
        print(f"{f.name:<14}{tag:>11}"
              f"{fmt(a['rounded']):>14}{fmt(a['ulp']):>13}"
              f"{fmt(a['abs_err']):>13}{a['rel_err']:>11.2e}{digs:>8}"
              f"  [{fmt(lo)}, {fmt(hi)}]")
    print()


# --------------------------------------------------------------------------- #
#  Visualizations (need matplotlib)
# --------------------------------------------------------------------------- #
def plot_range_coverage(formats, targets, outdir):
    """Fig 1: representable value range of each format (log x-axis), with the
    target values drawn as vertical lines.

    double's true range spans ~630 orders of magnitude (4.9e-324 .. 1.8e308); no
    single plot can show it together with fp4 (0.5..6) legibly. We use a finite
    display window [1e-45, 1e40] (which already fully contains every format except
    double); double's bar is clipped to the edge and its real extremes are written
    as text, so the figure stays honest without overflowing the log transform.
    """
    XMIN, XMAX = 1e-45, 1e40
    fig, ax = plt.subplots(figsize=(13, 6))
    colors = plt.cm.viridis(np.linspace(0.15, 0.85, len(formats)))
    for i, (f, c) in enumerate(zip(formats, colors)):
        lo = max(f.min_subnormal, XMIN)
        hi = min(f.max_finite, XMAX)
        mn = min(max(f.min_normal, XMIN), hi)   # clip min-normal into the window too
        ax.fill_betweenx([i - 0.35, i + 0.35], lo, hi, color=c, alpha=0.35)
        # Subnormal band a bit darker: it is where precision degrades.
        ax.fill_betweenx([i - 0.35, i + 0.35], lo, mn, color=c, alpha=0.55)
        ax.plot([mn, mn], [i - 0.35, i + 0.35], color=c, lw=2.2)  # min-normal tick
        # True max value at the right end (so clipped formats still read correctly).
        ax.text(hi, i, f"  {fmt(f.max_finite, 1)}",
                va="center", ha="left", fontsize=7, color=c)
    ax.set_yticks(range(len(formats)))
    ax.set_yticklabels([f"{f.name}  ({f.total_bits}bit)" for f in formats])
    ax.set_xscale("log")
    ax.set_xlim(XMIN, XMAX, auto=False)
    ax.set_xlabel("representable value (log scale)  -> larger")
    ax.set_title("Value-range coverage of each float format\n"
                 "(light = normal, dark = subnormal, tick = min normal; "
                 "red dashes = target values)")
    for t in targets:
        ax.axvline(t, color="red", ls="--", lw=1, alpha=0.7)
        ax.text(t, len(formats) - 0.4, f" {t:g}", color="red", va="bottom",
                ha="left", fontsize=9)
    ax.grid(True, which="both", axis="x", alpha=0.25)
    ax.invert_yaxis()
    fig.tight_layout()
    path = os.path.join(outdir, "01_value_range.png")
    fig.savefig(path, dpi=130)
    plt.close(fig)
    return path


def plot_ulp_staircase(formats, targets, outdir, n=6000):
    """Fig 2: ULP vs magnitude (log-log) -- the classic staircase."""
    fig, ax = plt.subplots(figsize=(12, 7))
    with np.errstate(over="ignore", under="ignore", invalid="ignore"):
        for f in formats:
            lo = max(f.min_subnormal * 1.0000001, _PLOT_XMIN)
            hi = min(f.max_finite, _PLOT_XMAX)
            if hi <= lo:
                continue
            xs = np.geomspace(lo, hi, n)
            ulps = np.array([f.ulp_at(float(v)) for v in xs])
            ax.loglog(xs, ulps, lw=1.4, label=f"{f.name}  ({f.decimal_digits:.1f} dig)")
        for t in targets:
            ax.axvline(t, color="red", ls="--", lw=1, alpha=0.6)
            ax.text(t, 0.02, f"{t:g}", color="red",
                    rotation=90, va="bottom", ha="right", fontsize=9,
                    transform=ax.get_xaxis_transform())
    ax.set_xlabel("value magnitude (log scale)")
    ax.set_ylabel("ULP = gap between adjacent representable values (log scale)")
    ax.set_title("ULP grows as a staircase with magnitude\n"
                 "flat within a binade, doubles at each power of two")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(loc="lower right", fontsize=9)
    fig.tight_layout()
    path = os.path.join(outdir, "02_ulp_staircase.png")
    fig.savefig(path, dpi=130)
    plt.close(fig)
    return path


def plot_relative_precision(formats, targets, outdir, n=6000):
    """Fig 3: usable decimal digits = -log10(ULP/value). Higher plateau = better."""
    fig, ax = plt.subplots(figsize=(12, 7))
    with np.errstate(over="ignore", under="ignore", invalid="ignore"):
        for f in formats:
            lo = max(f.min_subnormal * 1.0000001, _PLOT_XMIN)
            hi = min(f.max_finite, _PLOT_XMAX)
            if hi <= lo:
                continue
            xs = np.geomspace(lo, hi, n)
            digits = np.array([-math.log10(f.ulp_at(float(v)) / float(v)) for v in xs])
            ax.semilogx(xs, digits, lw=1.4, label=f.name)
            ax.axhline(f.decimal_digits, color=ax.lines[-1].get_color(),
                       ls=":", lw=0.8, alpha=0.5)
        for t in targets:
            ax.axvline(t, color="red", ls="--", lw=1, alpha=0.6)
    ax.set_xlabel("value magnitude (log scale)")
    ax.set_ylabel("usable decimal digits")
    ax.set_title("Relative precision (usable decimal digits) vs magnitude\n"
                 "plateau height ~= mant_bits*log10(2); collapses in the subnormal range")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(loc="lower center", fontsize=9)
    fig.tight_layout()
    path = os.path.join(outdir, "03_relative_precision.png")
    fig.savefig(path, dpi=130)
    plt.close(fig)
    return path


# --------------------------------------------------------------------------- #
#  Entry point
# --------------------------------------------------------------------------- #
def main():
    # 1) Text overview
    print_properties_table(FORMATS)

    # 2) Precision analysis for each target value
    for x in TARGETS:
        print_target_analysis(FORMATS, x)

    # 3) Visualizations
    if not _HAS_PLOT:
        print("[plots skipped] matplotlib not installed; text analysis is complete. "
              "Run setup_venv.sh to install it.")
        return

    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
    os.makedirs(outdir, exist_ok=True)
    print("Writing figures to:", outdir)
    print("  -", plot_range_coverage(FORMATS, TARGETS, outdir))
    print("  -", plot_ulp_staircase(FORMATS, TARGETS, outdir))
    print("  -", plot_relative_precision(FORMATS, TARGETS, outdir))
    print("\nTip: add plt.show() to view interactively, or just open the PNGs above.")


if __name__ == "__main__":
    main()
