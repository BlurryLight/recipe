%module testLib
%include "std_string.i"
%inline %{
    #include "testLib.h"
%}

%include "testLib.h"

