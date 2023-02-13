%module testLib
%include "std_string.i"
%include "std_vector.i"
%include "stl.i"

namespace std {
    %template(IntVector)    vector<int>;
    %template(DoubleVector) vector<double>;
}

%inline %{
    #include "testLib.h"
%}

%include <testLib.h>