这个文件夹用来测试MSVC一个关于同名符号在不同的动态链接库和静态链接库下的链接问题。

结论:

1. 当头文件出现同一个函数声明，而在不同的library里有不同的实现的时候。
不会出现链接错误，会根据第一个出现的实现进行链接。
无论静态库还是动态库。

2. 在同一个library里出现同一个函数的不同实现，会违反ODR。