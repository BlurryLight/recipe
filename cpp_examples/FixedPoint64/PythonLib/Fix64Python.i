%module Fix64

%include "stdint.i"
%include "typemaps.i"

%rename(Fix64_long) Fix64(int64_t);
%rename(toFloat) operator float();
%rename(toLong) operator int64_t();
%rename(toDouble) operator double();
%ignore operator!();
%inline %{
    #include "Fix64.h"
%}

%include <Fix64.h>