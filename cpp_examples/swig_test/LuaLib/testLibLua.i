%module testLib
%include "std_string.i"
%include "std_vector.i"
%include "stl.i"

// using typemaps
// see https://github.com/swig/swig/blob/master/Lib/lua/typemaps.i
%include <typemaps.i>
%apply (double *INPUT,int) {(const double* arr,int len)};
%apply (float *INOUT, int) {(float *io1, int n1)};

namespace std {
    %template(IntVector)    vector<int>;
    %template(DoubleVector) vector<double>;
}
%inline %{
    #include "testLib.h"
%}


%include "testLib.h"