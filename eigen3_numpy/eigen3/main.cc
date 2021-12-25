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

void example2() {
  // slicing
  // https://eigen.tuxfamily.org/dox/group__TutorialSlicingIndexing.html
  {
    Eigen::MatrixXd a;
    a.resize(3, 4);
    // clang-format off
    a << 1, 2, 3, 4,
       5, 6, 7, 8,
       9, 10, 11, 12;
    // clang-format on

    // eigen 3.4 support a much improved api
    // see https://eigen.tuxfamily.org/dox/group__TutorialSlicingIndexing.html

    // old-style
    //  operator= always gives a deep copy
    // if use auto it will create a reference
    Eigen::MatrixXd b = a.block(0, 1, 2, 2);
    Eigen::Ref<Eigen::MatrixXd> d = a.block(0, 1, 2, 2);
    std::cout << b << std::endl;
    // after 3.4
    // Eigen::seq [begin,end]
    Eigen::MatrixXd c = a(Eigen::seq(0, 1), Eigen::seq(1, 2));
    std::cout << c << std::endl;
    fmt::print("c.cwiseEqual b {},c == b {},array equal = {}",
               c.cwiseEqual((b)), (c == b), c.array() == b.array());

    // modifying original block
    std::cout << a << std::endl;
    b(0, 1) = 1000;
    std::cout << a << std::endl;
    c(0, 1) = -1000;
    std::cout << a << std::endl;
    d(0, 1) = -1000;
    std::cout << a << std::endl;
  }
  {
    std::cout << "=============" << std::endl;
    Eigen::MatrixXd a;
    a.resize(3, 4);
    // clang-format off
    a << 1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12;
    // clang-format on
    std::cout << a(1, Eigen::all) << " rows: " << a(1, Eigen::all).rows()
              << " Cols: " << a(1, Eigen::all).cols() << std::endl;
    Eigen::VectorXd d = a.row(1); // interesting behaviour
    fmt::print("{},rows {} cols {}\n", d, d.rows(), d.cols());
    Eigen::RowVectorXd rd = a.row(1);
    fmt::print("{},rows {} cols {}\n", rd, rd.rows(), rd.cols());
  }

  {
    // integer slicing
    std::cout << "=============" << std::endl;
    Eigen::MatrixXd a;
    a.resize(3, 2);
    // clang-format off
    a << 1, 2,
        3,4,
        5,6;
    // clang-format on
    // Eigen doesn't support print(a[[0, 1, 2], [0, 1, 0]]) this way
    // it should be
    Eigen::RowVector3d b;
    b << a(0, 0), a(1, 1), a(2, 0);
    std::cout << b << std::endl;
  }

  {
    // bool index
    std::cout << "=============" << std::endl;
    Eigen::MatrixXd a;
    a.resize(3, 2);
    // clang-format off
    a << 1, 2,
        3,4,
        5,6;
   std::cout << (a.array() > 3) << std::endl;
    a = (a.array() > 3).select(3, a);
    std::cout << a << std::endl;
  }
}

int main() {
  fmt::print("Eigen Version: {}.{}.{}", EIGEN_WORLD_VERSION,
             EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION);
  static_assert(EIGEN_MAJOR_VERSION >= 4, "Eigen Version must >= 3.4.x");
  example1();
  example2();
  return 0;
}
