%module Fix64

// hack for win-python
// msvc will try to link python_d.lib which is not default installed
%begin %{
#ifdef _MSC_VER
#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
#endif
%}

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