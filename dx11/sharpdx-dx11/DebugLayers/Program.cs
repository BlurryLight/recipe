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
using System.Windows.Forms;
using SharpDX;
using SharpDX.Direct3D;
using SharpDX.Direct3D11;
using SharpDX.DXGI;
using SharpDX.Windows;
using Buffer = SharpDX.Direct3D11.Buffer;
using Device = SharpDX.Direct3D11.Device;

namespace DebugLayers
{
    /// <summary>
    ///   SharpDX port of SharpDX-MiniTri Direct3D 11 Sample
    /// </summary>
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
#if DEBUG
            SharpDX.Configuration.EnableObjectTracking = true;
#endif

            #region Direct3D Init

            String OriginalText = "SharpDx D3D11.2";
            var form = new RenderForm(OriginalText)
            {
                ClientSize = new Size(1600, 800)
            };

            // Create Device and SwapChain
            Device device;
            SwapChain swapChain;
            Device.CreateWithSwapChain(
                SharpDX.Direct3D.DriverType.Hardware,
                // Enable Device debug layer
                DeviceCreationFlags.Debug,
                new[]
                {
                    SharpDX.Direct3D.FeatureLevel.Level_11_0,
                    SharpDX.Direct3D.FeatureLevel.Level_10_1,
                    SharpDX.Direct3D.FeatureLevel.Level_10_0,
                },
                new SwapChainDescription()
                {
                    ModeDescription =
                        new ModeDescription(
                            form.ClientSize.Width,
                            form.ClientSize.Height,
                            new Rational(60, 1),
                            Format.R8G8B8A8_UNorm
                        ),
                    SampleDescription = new SampleDescription(1, 0),
                    Usage = SharpDX.DXGI.Usage.BackBuffer | Usage.RenderTargetOutput,
                    BufferCount = 1,
                    Flags = SwapChainFlags.None,
                    IsWindowed = true,
                    OutputHandle = form.Handle,
                    SwapEffect = SwapEffect.Discard
                },
                out device, out swapChain
            );

            var context = device.ImmediateContext;

            // New RenderTargetView from the backbuffer
            var backBuffer = Texture2D.FromSwapChain<Texture2D>(swapChain, 0);
            // 要使用buffer必须创建对应的view，所有的操作都是对view进行操作
            var renderTargetView = new RenderTargetView(device, backBuffer);

            device.DebugName = "Dx11 Device";
            swapChain.DebugName = "swchain";
            backBuffer.DebugName = "backbuffer";
            renderTargetView.DebugName = "rtv";

            #endregion


            #region D3d Loop

            var clock = new System.Diagnostics.Stopwatch();
            var clockFrequency = (double) System.Diagnostics.Stopwatch.Frequency;
            clock.Start();
            var deltaTime = 0.0;
            var fpsTimer = new System.Diagnostics.Stopwatch();
            fpsTimer.Start();
            var fps = 0.0;
            int fpsFrames = 0;

            // Main loop
            RenderLoop.Run(form, () =>
            {
                var totalSeconds = clock.ElapsedTicks / clockFrequency;

                #region FPS calc

                fpsFrames++;
                if (fpsTimer.ElapsedMilliseconds > 1000)
                {
                    fps = 1000.0 * fpsFrames / fpsTimer.ElapsedMilliseconds;
                    //$ 表示格式化，@表示支持多行
                    form.Text =
                        $@"{OriginalText} - FPS: {fps:F2} ({(float) fpsTimer.ElapsedMilliseconds / fpsFrames:F2}ms/frame)";
                    fpsFrames = 0;
                    fpsTimer.Reset();
                    fpsTimer.Restart();
                }

                #endregion

                var lerpColor = SharpDX.Color.Lerp(SharpDX.Color.LightBlue, SharpDX.Color.DarkBlue,
                    (float) ((totalSeconds / 2.0) % 1.0));
                context.ClearRenderTargetView(renderTargetView, lerpColor);
                //internally use Present1 method
                swapChain.Present(0, 0);
                //intentionally bug
                // swapChain.Present(0, PresentFlags.RestrictToOutput);
                deltaTime = (clock.ElapsedMilliseconds / clockFrequency) - totalSeconds;
            });

            #endregion

#if DEBUG
            Console.WriteLine($@"Active Sharpdx Objects {SharpDX.Diagnostics.ObjectTracker.ReportActiveObjects()}");
#endif

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