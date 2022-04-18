# msbuild的一些测试

https://github.com/MicrosoftDocs/visualstudio-docs/issues/2774#issuecomment-489685855
`Directory.Build.props`: 会在vcxproj刚开始的时候被导入，定义在里面的属性可能被覆盖。
`Directory.Build.targets`:在结尾导入，所以会覆盖默认的属性，一般选这个。

可以通过`targets`里定义的属性覆盖`vcxproj`内定义的属性。
见`Directory.Build.targets`内的`ItemDefinitnionGroup`，定义了`PreprocessorDefinitions`。
