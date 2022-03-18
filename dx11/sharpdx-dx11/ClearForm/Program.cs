// Copyright (c) 2010-2013 SharpDX - Alexandre Mutel
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// -----------------------------------------------------------------------------
// Original code from SlimDX project.
// Greetings to SlimDX Group. Original code published with the following license:
// -----------------------------------------------------------------------------
/*
* Copyright (c) 2007-2011 SlimDX Group
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
using System;
using System.Drawing;
using SharpDX;
using SharpDX.Direct3D;
using SharpDX.Direct3D11;
using SharpDX.DXGI;
using SharpDX.Windows;
using Buffer = SharpDX.Direct3D11.Buffer;
using Device = SharpDX.Direct3D11.Device;

namespace ClearForm
{
    /// <summary>
    ///   SharpDX port of SharpDX-MiniTri Direct3D 11 Sample
    /// </summary>
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            #region Direct3D Init

            var form = new RenderForm("SharpDX - MiniTri Direct3D 11 Sample")
            {
                ClientSize = new Size(1600, 800)
            };

            // SwapChain description
            var desc = new SwapChainDescription
            {
                               BufferCount = 1,
                               ModeDescription= 
                                   new ModeDescription(form.ClientSize.Width, form.ClientSize.Height,
                                       // UNORM代表unsigned_normalized
                                                       new Rational(60, 1), Format.R8G8B8A8_UNorm),
                               IsWindowed = true,
                               OutputHandle = form.Handle,
                               //disable MSAA
                               SampleDescription = new SampleDescription(1, 0),
                               // after swapchains present, the backbuffer is discarded
                               SwapEffect = SwapEffect.Discard,
                               Usage = Usage.RenderTargetOutput | Usage.BackBuffer,
                               Flags = SwapChainFlags.None
                           };

            // Create Device and SwapChain
            Device device;
            SwapChain swapChain;
            //静态方法，把创建出来的device和swapchain放入上面的两个变量(引用
            //DeviceCreationFlags = D3D11_CREATE_DEVICE_FLAG
            Device.CreateWithSwapChain(DriverType.Hardware, DeviceCreationFlags.None,new [] {FeatureLevel.Level_11_1,FeatureLevel.Level_11_0}, desc, out device, out swapChain);
            var context = device.ImmediateContext;

            // New RenderTargetView from the backbuffer
            var backBuffer = Texture2D.FromSwapChain<Texture2D>(swapChain, 0);
            // 要使用buffer必须创建对应的view，所有的操作都是对view进行操作
            var renderTargetView = new RenderTargetView(device, backBuffer);
            #endregion

            #region D3d Loop
            // Main loop
            RenderLoop.Run(form, () =>
            {
                Int32 unixTimestamp = (int)DateTime.UtcNow.Subtract(new DateTime(1970, 1, 1)).TotalSeconds;
                // In Project Settings change the application Type as Console to allow Console.Writeline
                // Console.WriteLine(unixTimestamp);
                var lerpColor = SharpDX.Color.Lerp(SharpDX.Color.LightBlue, SharpDX.Color.DarkBlue,
                    (float) ((unixTimestamp % 30) / 30.0));
                                          context.ClearRenderTargetView(renderTargetView, lerpColor);
                                          swapChain.Present(0, PresentFlags.None);
                                      });
            #endregion

            // Release all resources
            #region D3d Clean
            renderTargetView.Dispose();
            backBuffer.Dispose();
            context.ClearState();
            context.Flush();
            device.Dispose();
            context.Dispose();
            swapChain.Dispose();
            #endregion
        }
    }
}