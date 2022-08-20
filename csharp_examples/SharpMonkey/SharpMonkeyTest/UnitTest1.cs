using NUnit.Framework;
using SharpMonkey;

namespace SharpMonkeyTest
{
    public class Tests
    {
        [SetUp]
        public void Setup()
        {
        }

        [Test]
        public void Test1()
        {
            Assert.IsTrue(A.foo());
            Assert.Pass();
        }
    }
}