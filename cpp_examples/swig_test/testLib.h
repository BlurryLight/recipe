#pragma once
#include <string>
int add(int a,int b);

struct Vec3f
{
    float x,y,z;
    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float x,float y,float z) : x(x), y(y), z(z) {}
    Vec3f operator+(const Vec3f &other);
    void Display();
    std::string Text();
};

class Shape
{
public:
  static int nShapes;
  Shape(){
    nShapes++;
  }
  virtual ~Shape(){
    nShapes--;
  }
  virtual double area() = 0;
};

class Circle : public Shape
{
  private:
    double r = 0;
  public:
    Circle(double radius) : r(radius){}
    virtual double area() override ;
};
