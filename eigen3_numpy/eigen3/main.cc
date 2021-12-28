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

    // map matrix to vector

    Eigen::Map<Eigen::RowVectorXd> v1(a.data(), a.size());
    fmt::print("v1: {}\n", v1); // because a is col-major

    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> a2(
        a);
    Eigen::Map<Eigen::RowVectorXd> v2(a2.data(), a2.size());
    fmt::print("v2: {}\n", v2); // a2 is row-major
    // reshpae

    using RowMatrixXd =
        Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    Eigen::Map<RowMatrixXd> a3(a2.data(), 6, 2);
    fmt::print("a3: {}\n", a3); // row-major to row major

    Eigen::Map<Eigen::MatrixXd> a4(a.data(), 6, 2);
    fmt::print("a4: {}\n", a3); // col-major to col-major
    // after 3.4 api
    Eigen::MatrixXd a5 = a.reshaped(6, 2);
    fmt::print("a5: {}\n", a5); // interesing-behavour
    RowMatrixXd a6 = a2.reshaped<Eigen::RowMajor>(6, 2);
    fmt::print("a6: {}\n", a6);

    fmt::print("a5 vector view: {}\n", a.reshaped().transpose()); // vector-view
    fmt::print("a6 vector view: {}\n",
               a.reshaped<Eigen::RowMajor>().transpose()); // vector-view
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
    // clang-format on
    std::cout << (a.array() > 3) << std::endl;
    a = (a.array() > 3).select(3, a);
    std::cout << a << std::endl;
  }
}
void example3() {
  Eigen::Matrix2d x;
  x << 1, 2, 3, 4;
  Eigen::Matrix2d y;
  y << 5, 6, 7, 8;
  fmt::print("x - y :\n{}\n", x - y);
  fmt::print("x + y :\n{}\n", x + y);
  fmt::print("x * y :\n{}\n", x.array() * y.array());
  fmt::print("x / y :\n{}\n", x.array() / y.array());
  fmt::print("np.sqrt(x):\n{}\n", x.cwiseSqrt());

  Eigen::Vector2d v;
  Eigen::Vector2d w;
  v << 9, 10;
  w << 11, 12;
  fmt::print("v.dot(w):\n{}\n", v.dot(w));
  fmt::print("x.dot(v):\n{}\n", x * v);
  fmt::print("x.dot(y):\n{}\n", x * y);

  fmt::print("np.sum(x):\n{}\n", x.sum());
  fmt::print("np.sum(x,axis=0):\n{}\n", x.colwise().sum());
  fmt::print("np.sum(x,axis=1):\n{}\n", x.rowwise().sum());
  fmt::print("np.max(x,axis=1):\n{}\n", x.rowwise().maxCoeff());
  fmt::print("np.max(x,axis=0):\n{}\n", x.colwise().maxCoeff());
  //寻找和最大的列向量
  Eigen::MatrixXd mat(2, 4);
  mat << 1, 2, 6, 9, 3, 1, 7, 2;
  Eigen::MatrixXd::Index idx;
  float maxNorm = mat.colwise().sum().maxCoeff(&idx);
  fmt::print("max col-vector:\n{} and its sum is {}\n", mat.col(idx), maxNorm);
  fmt::print("np.transpose(x):\n{}\n", x.transpose());
}
void example4() {
  // broadcasting
  Eigen::MatrixXd mat(4, 3);
  mat << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12;
  Eigen::Vector3d v(1, 0, 1);
  Eigen::MatrixXd y = mat.rowwise() + v.transpose();
  fmt::print("y=\n{}\n", y);
  Eigen::RowVector3d v1(v.data());
  fmt::print("np.tile(v,(4,1))\n{}\n", v1.replicate<4, 1>());
  y = v1.replicate<4, 1>();
  fmt::print("y=\n{}\n", y);
  fmt::print("mat + y\n{}\n", mat + y);
  // outer product
  auto u = Eigen::Vector3d(1, 2, 3);
  auto w = Eigen::Vector2d(4, 5);
  fmt::print("u outproduct w:\n {}\n", u * w.transpose());
}

int main() {
  fmt::print("Eigen Version: {}.{}.{}\n", EIGEN_WORLD_VERSION,
             EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION);
  static_assert(EIGEN_MAJOR_VERSION >= 4, "Eigen Version must >= 3.4.x");
  //  example1();
  //  example2();
  //  example3();
  example4();

  return 0;
}
