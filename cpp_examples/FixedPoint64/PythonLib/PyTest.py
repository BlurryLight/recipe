#!/usr/bin/env python3
import unittest
import Fix64
import random


random.seed(1234.0)


class TestBasicFix64(unittest.TestCase):

    # def __init__(self,):
    #     super().__init__()

    def test_create(self):
        f = Fix64.Fix64()
        self.assertAlmostEqual(f.toFloat(), 0.0)

        f = Fix64.Fix64_FromInt64(1)
        self.assertAlmostEqual(f.toFloat(), 1.0)
        self.assertAlmostEqual(f.toInt64(), 1)

        f = Fix64.Fix64_FromInt64(-1)
        self.assertAlmostEqual(f.toFloat(), -1.0)
        self.assertAlmostEqual(f.toInt64(), -1)

        f = Fix64.Fix64_FromFloat(3.5)
        self.assertAlmostEqual(f.toFloat(), 3.5)
        self.assertAlmostEqual(f.toInt64(), 4)

        f = Fix64.Fix64_FromDouble(4.3)
        self.assertAlmostEqual(f.toDouble(), 4.3)
        self.assertAlmostEqual(f.toInt64(), 4)

    def test_special_val(self):
        fix64_max = Fix64.Fix64_FromRaw(Fix64.Fix64.kMax)
        fix64_min = Fix64.Fix64_FromRaw(Fix64.Fix64.kMin)
        fix64_one = Fix64.Fix64_FromInt64(1)
        fix64_delta = Fix64.Fix64_FromRaw(1)

        self.assertEqual(fix64_max * fix64_one, fix64_max)
        self.assertEqual(fix64_max + fix64_delta, fix64_min)

    def test_constants(self):
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kOne).toFloat(), 1)
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kHalf).toFloat(), 0.5)
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kMax).toDouble(), pow(2, Fix64.kIntegerBit - 1))
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kMin).toDouble(), -pow(2, Fix64.kIntegerBit - 1))

    def test_compare(self):
        a = Fix64.Fix64_FromInt64(1)
        b = Fix64.Fix64_FromInt64(2)
        c = Fix64.Fix64_FromInt64(1)
        d = Fix64.Fix64_FromInt64(-1)
        self.assertEqual(a, c)
        self.assertNotEqual(a, b)
        self.assertEqual(a, -d)

        self.assertGreater(b, a)
        self.assertGreater(a, d)
        self.assertGreaterEqual(a, d)

        self.assertLess(a, b)
        self.assertLessEqual(a, b)

        self.assertTrue(a)
        self.assertTrue(b)
        self.assertTrue(c)
        self.assertTrue(d)
        self.assertFalse(Fix64.Fix64())

    def test_sign(self):
        a = Fix64.Fix64_FromInt64(2)
        b = Fix64.Fix64_FromInt64(0)
        c = Fix64.Fix64_FromInt64(-2)
        self.assertEqual(a.Sign(), 1)
        self.assertEqual((-a).Sign(), -1)
        self.assertEqual(b.Sign(), 0)
        self.assertEqual((-b).Sign(), 0)
        self.assertEqual(c.Sign(), -1)
        self.assertEqual((-c).Sign(), 1)

    def test_add(self):
        n = 1000
        a = [random.randint(-10000, 10000) for i in range(n)]
        b = [random.randint(-10000, 10000) for i in range(n)]
        c = [(a[i] + b[i]) for i in range(n)]
        for i in range(n):
            self.assertEqual(Fix64.Fix64_FromRaw(a[i]) + Fix64.Fix64_FromRaw(b[i]),
                             Fix64.Fix64_FromRaw(c[i]))
            self.assertEqual(Fix64.Fix64_FromRaw(a[i]).SafeAdd(Fix64.Fix64_FromRaw(b[i])),
                             Fix64.Fix64_FromRaw(c[i]))

    def test_minus(self):
        n = 1000
        a = [random.randint(-10000, 10000) for i in range(n)]
        b = [random.randint(-10000, 10000) for i in range(n)]
        c = [(a[i] - b[i]) for i in range(n)]
        for i in range(n):
            self.assertEqual(Fix64.Fix64_FromRaw(a[i]) - Fix64.Fix64_FromRaw(b[i]),
                             Fix64.Fix64_FromRaw(c[i]))
            self.assertEqual(Fix64.Fix64_FromRaw(a[i]).SafeMinus(Fix64.Fix64_FromRaw(b[i])),
                             Fix64.Fix64_FromRaw(c[i]))

    def test_mul(self):

        n = 1000
        a = [random.randrange(-10000, 10000) for _ in range(n)]
        b = [random.randrange(-10000, 10000) for _ in range(n)]
        c = [(a[i] * b[i]) for i in range(n)]
        for i in range(n):
            self.assertEqual(Fix64.Fix64_FromInt64(a[i]) * Fix64.Fix64_FromInt64(b[i]),
                             Fix64.Fix64_FromInt64(c[i]))
            self.assertAlmostEqual(Fix64.Fix64_FromDouble(a[i]) * Fix64.Fix64_FromDouble(b[i]),
                                   Fix64.Fix64_FromDouble(c[i]), None, None, Fix64.Fix64_FromFloat(0.005))

    def test_div(self):
        n = 1000
        a = [random.randrange(-10000, 10000) for _ in range(n)]
        b = [random.randrange(1, 10000) for _ in range(n//2)]
        b.extend([random.randrange(-10000, -1) for _ in range(n//2)])

        c = [(a[i] / b[i]) for i in range(n)]
        for i in range(n):
            self.assertAlmostEqual(Fix64.Fix64_FromInt64(a[i]) / Fix64.Fix64_FromInt64(b[i]),
                                   Fix64.Fix64_FromDouble(c[i]))

    def test_abs(self):
        min_fix = Fix64.Fix64_FromRaw(Fix64.Fix64.kMin)
        max_fix = Fix64.Fix64_FromRaw(Fix64.Fix64.kMax)
        self.assertEqual(min_fix.Abs(), max_fix)
        self.assertEqual(min_fix.FastAbs(), min_fix)
        self.assertEqual(max_fix.Abs(), max_fix)
        self.assertEqual(max_fix.FastAbs(), max_fix)

        zero = Fix64.Fix64.FromDouble(0)
        a = Fix64.Fix64.FromDouble(1.5)
        c = Fix64.Fix64.FromDouble(-1.0)
        self.assertEqual(zero.Abs(), zero)
        self.assertEqual(zero.FastAbs(), zero)
        self.assertEqual(a.Abs(), a)
        self.assertEqual(c.Abs(), Fix64.Fix64_FromDouble(1.0))
        self.assertEqual(a.FastAbs(), a)
        self.assertEqual(c.FastAbs(), Fix64.Fix64_FromDouble(1.0))


if __name__ == '__main__':
    unittest.main()
