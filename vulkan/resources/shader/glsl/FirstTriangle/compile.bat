@REM sadly there is no entry-point renaming in glslc
glslc shader.vert -g -O0 -fshader-stage=vert -o shader.vert.spirv 
glslc shader.frag -g -O0 -fshader-stage=frag -o shader.frag.spirv 
