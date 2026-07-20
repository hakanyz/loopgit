Add-Type -AssemblyName System.Drawing

$pngPath = "resources\logo.png"
$icoPath = "resources\app_icon.ico"

$code = @"
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

public class IcoGenerator {
    public static void ConvertToRoundedIco(string pngPath, string icoPath) {
        using (Bitmap orig = new Bitmap(pngPath)) {
            // Create a perfect circular version of the logo
            Bitmap circularLogo = new Bitmap(orig.Width, orig.Height);
            using (Graphics g = Graphics.FromImage(circularLogo)) {
                g.SmoothingMode = SmoothingMode.AntiAlias;
                g.Clear(Color.Transparent);
                
                // Draw a circle
                GraphicsPath path = new GraphicsPath();
                path.AddEllipse(0, 0, orig.Width, orig.Height);
                g.SetClip(path);
                
                // Draw the original image inside the circle
                g.DrawImage(orig, 0, 0, orig.Width, orig.Height);
            }

            using (FileStream fs = new FileStream(icoPath, FileMode.Create)) {
                using (BinaryWriter bw = new BinaryWriter(fs)) {
                    // Header
                    bw.Write((short)0); // reserved
                    bw.Write((short)1); // type 1 = ico
                    bw.Write((short)4); // 4 images: 256, 64, 48, 32
                    
                    int[] sizes = { 256, 64, 48, 32 };
                    int offset = 6 + (16 * sizes.Length);
                    
                    byte[][] images = new byte[sizes.Length][];
                    
                    for (int i = 0; i < sizes.Length; i++) {
                        int size = sizes[i];
                        Bitmap resized = new Bitmap(circularLogo, new Size(size, size));
                        using (MemoryStream ms = new MemoryStream()) {
                            resized.Save(ms, ImageFormat.Png);
                            images[i] = ms.ToArray();
                        }
                        
                        bw.Write((byte)(size == 256 ? 0 : size));
                        bw.Write((byte)(size == 256 ? 0 : size));
                        bw.Write((byte)0); // colors
                        bw.Write((byte)0); // reserved
                        bw.Write((short)1); // planes
                        bw.Write((short)32); // bpp
                        bw.Write((int)images[i].Length); // size
                        bw.Write((int)offset); // offset
                        offset += images[i].Length;
                    }
                    
                    for (int i = 0; i < sizes.Length; i++) {
                        bw.Write(images[i]);
                    }
                }
            }
        }
    }
}
"@

Add-Type -TypeDefinition $code -ReferencedAssemblies System.Drawing
[IcoGenerator]::ConvertToRoundedIco($pngPath, $icoPath)
Write-Host "Rounded Icon generated successfully!"
