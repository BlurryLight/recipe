#!/usr/bin/env python3
import unittest
import Fix64


class TestBasicFix64(unittest.TestCase):

    def test_create(self):
        f = Fix64.Fix64()
        self.assertAlmostEqual(f.toFloat(), 0.0)

        f = Fix64.Fix64_long(1)
        self.assertAlmostEqual(f.toFloat(), 1.0)
        self.assertAlmostEqual(f.toLong(), 1)

        f = Fix64.Fix64_long(-1)
        self.assertAlmostEqual(f.toFloat(), -1.0)
        self.assertAlmostEqual(f.toLong(), -1)

        f = Fix64.Fix64_FromFloat(3.5)
        self.assertAlmostEqual(f.toFloat(), 3.5)
        self.assertAlmostEqual(f.toLong(), 4)

        f = Fix64.Fix64_FromDouble(4.3)
        self.assertAlmostEqual(f.toDouble(), 4.3)
        self.assertAlmostEqual(f.toLong(), 4)

    def test_constants(self):
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kOne).toFloat(), 1)
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kHalf).toFloat(), 0.5)

    def test_compare(self):
        a = Fix64.Fix64_long(1)
        b = Fix64.Fix64_long(2)
        c = Fix64.Fix64_long(1)
        d = Fix64.Fix64_long(-1)
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
        a = Fix64.Fix64_long(2)
        b = Fix64.Fix64_long(0)
        c = Fix64.Fix64_long(-2)
        self.assertEqual(a.sign(), 1)
        self.assertEqual((-a).sign(), -1)
        self.assertEqual(b.sign(), 0)
        self.assertEqual((-b).sign(), 0)
        self.assertEqual(c.sign(), -1)
        self.assertEqual((-c).sign(), 1)

    def test_binary(self):
        a = Fix64.Fix64.FromDouble(1.5)
        b = Fix64.Fix64.FromDouble(2.5)
        c = Fix64.Fix64.FromDouble(-1.0)

        self.assertEqual(a + b, Fix64.Fix64_long(4))
        self.assertEqual(a - b, Fix64.Fix64_long(-1))
        self.assertEqual(a - b, c)


if __name__ == '__main__':
    unittest.main()
