%module Fix64

// hack for win-python
// msvc will try to link python_d.lib which is not default installed
%begin %{
#ifdef _MSC_VER
#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
#endif
%}

%include "stdint.i"
%include "std_string.i"
%include "typemaps.i"

%rename(Fix64_long) Fix64(int64_t);
%rename(toFloat) operator float() const;
%rename(toLong) operator int64_t() const;
%rename(toDouble) operator double() const;
%ignore operator!() const;
%inline %{
    #include "Fix64.h"
%}

%include <Fix64.h>


%extend Fix64{
  std::string __repr__() {
    return $self->GetDesc();
  }
  Fix64 __abs__() {
    return $self->Abs();
  }
};