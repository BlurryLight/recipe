using System;
using System.Drawing;
using System.Windows.Forms;
using SharpDX;
using SharpDX.Direct3D;
using SharpDX.Direct3D11;
using SharpDX.DXGI;
using SharpDX.Windows;

using Common;
namespace D3DAPP
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {

            String OriginalText = "SharpDx D3D11.2";
            var form = new RenderForm(OriginalText)
            {
                ClientSize = new Size(800, 600)
            };

            using (D3DApp app = new D3DApp(form))
            {
                // Only render frames at the maximum rate the
                // display device can handle.
                app.VSync = true;

                // Initialize the framework (creates Direct3D device etc)
                app.Initialize();

                // Run the application
                app.Run();
            }

        }
    }
}
