#include <Eigen/Dense>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iostream>

using namespace std;

void example1()
{
  //init and pick
    {
        Eigen::VectorXd a(3);
        a << 1, 2, 3;
        fmt::print("{} {} {}\n", a[0], a[1], a[2]);
        a[0] = 5;
        fmt::print("{} {} {}\n", a[0], a[1], a[2]);

        Eigen::Vector3d b(1, 2, 3);
        fmt::print("{} {} {}\n", b[0], b[1], b[2]);
        b[0] = 5;
        fmt::print("{} {} {}\n", b[0], b[1], b[2]);
    }

    {
        Eigen::MatrixXd a(2, 3);
        a << 1, 2, 3, 4, 5, 6;
        std::cout << a << std::endl;
        fmt::print("\n{} {} {}\n",a.rows(),a.cols(),a.size()); //2 3 6
        fmt::print("\n{} {} {}\n",a(0,0),a(0,1),a(1,0));
    }

    {
      auto a = Eigen::MatrixXd::Zero(2,2);
      std::cout << a << std::endl;
      auto b = Eigen::MatrixXd::Ones(1,2);
      std::cout << b << std::endl;
      auto c = Eigen::MatrixXd::Constant(2,2,7);
      std::cout << c << std::endl;
      auto d = Eigen::MatrixXd::Identity(2,2);
      std::cout << d << std::endl;
      //np random [0,1]
      //eigen random [-1,1]
      //array and matrix can implicit cast
      Eigen::MatrixXd e = (Eigen::MatrixXd::Random(2,2).array() + 1.0) * 0.5;
      std::cout << e << std::endl;
    }
}

void example2()
{
  //slicing
}

int main()
{
    example1();
    return 0;
}
