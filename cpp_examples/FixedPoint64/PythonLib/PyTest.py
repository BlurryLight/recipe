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

        f = Fix64.Fix64_FromFloat(3.5)
        self.assertAlmostEqual(f.toFloat(), 3.5)
        self.assertAlmostEqual(f.toLong(), 4)

        f = Fix64.Fix64_FromDouble(4.3)
        self.assertAlmostEqual(f.toDouble(), 4.3)
        self.assertAlmostEqual(f.toLong(), 4)

    def test_constants(self):
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kOne).toFloat(), 1)
        self.assertAlmostEqual(Fix64.Fix64_FromRaw(Fix64.Fix64.kHalf).toFloat(), 0.5)


if __name__ == '__main__':
    unittest.main()
