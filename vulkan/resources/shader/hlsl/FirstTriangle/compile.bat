@REM https://www.youtube.com/watch?v=Fja4lT508cA
@REM the flags in that video/slide don't work well in renderdoc. BUT it DOES provide more details in SPIRV.
%VULKAN_SDK%\Bin\dxc.exe -T vs_6_0 -E VSMain -spirv -Zi -Od shader.hlsl  -Fo  shader.vert.spirv
%VULKAN_SDK%\Bin\dxc.exe -T ps_6_0 -E PSMain -spirv -Zi -Od shader.hlsl  -Fo  shader.frag.spirv
