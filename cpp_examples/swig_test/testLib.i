%module testLib
%inline %{
    extern int add(int a,int b);
%}

