param(
    [string]$Destination = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ([string]::IsNullOrWhiteSpace($Destination)) {
    $Destination = Join-Path (Split-Path -Parent $PSScriptRoot) 'Saved\ARMEMENT01\ARMEMENT01_6_3\Icons'
}
New-Item -ItemType Directory -Path $Destination -Force | Out-Null

# All raster artwork is generated locally from this code. No third-party
# converter, remote API, or bundled font is required. Uses Windows GDI+.
Add-Type -ReferencedAssemblies @('System.Drawing') -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

public static class ARM0163PilotIcons
{
    static Color C(int r, int g, int b) { return Color.FromArgb(255,r,g,b); }
    static PointF[] P(params float[] v) {
        PointF[] p = new PointF[v.Length / 2];
        for (int i=0; i<p.Length; ++i) p[i] = new PointF(v[i*2],v[i*2+1]);
        return p;
    }
    static void Poly(Graphics g, PointF[] pts, Color top, Color bot, Color edge, float thickness) {
        using (GraphicsPath path = new GraphicsPath()) {
            path.AddPolygon(pts);
            RectangleF rect = path.GetBounds();
            using (LinearGradientBrush b = new LinearGradientBrush(
                new PointF(rect.Left,rect.Top),new PointF(rect.Right+0.01f,rect.Bottom+0.01f),top,bot)) g.FillPath(b,path);
            using (Pen p = new Pen(edge,thickness)) { p.LineJoin=LineJoin.Round; g.DrawPath(p,path); }
        }
    }
    static void Stroke(Graphics g, Color color, float width, params float[] values) {
        PointF[] pts=P(values);
        using (Pen p=new Pen(color,width)) { p.LineJoin=LineJoin.Round; p.StartCap=LineCap.Round; p.EndCap=LineCap.Round; g.DrawLines(p,pts); }
    }
    static void Ellipse(Graphics g, float x,float y,float w,float h, Color inside,Color edge,float thick) {
        using(Brush b=new SolidBrush(inside)) g.FillEllipse(b,x,y,w,h);
        using(Pen p=new Pen(edge,thick)) g.DrawEllipse(p,x,y,w,h);
    }
    static Bitmap Canvas(out Graphics g) {
        Bitmap bmp=new Bitmap(512,512,PixelFormat.Format32bppArgb);
        bmp.SetResolution(96,96);
        g=Graphics.FromImage(bmp);
        g.Clear(Color.Transparent);
        g.SmoothingMode=SmoothingMode.AntiAlias;
        g.InterpolationMode=InterpolationMode.HighQualityBicubic;
        g.PixelOffsetMode=PixelOffsetMode.HighQuality;
        return bmp;
    }
    public static void Dagger(string path) {
        Graphics g; using(Bitmap bmp=Canvas(out g)) {
            using(g) {
                g.TranslateTransform(256,256);
                g.RotateTransform(39.0f);
                // leather wrapped grip & patinated bronze bands
                Poly(g,P(-13,20,13,20,12,145,-12,145),C(111,64,38),C(46,26,24),C(25,20,24),6);
                for(int i=0;i<7;i++) Stroke(g,C(168,121,66),4,-10,37+i*15,10,51+i*15);
                Poly(g,P(-17,143,17,143,12,159,-12,159),C(197,150,79),C(87,64,46),C(29,27,30),5);
                Ellipse(g,-16,151,32,30,C(70,125,142),C(229,190,113),5);
                // curved, forged crossguard
                Poly(g,P(-74,9,-63,-4,-18,2,0,-6,18,2,62,-4,74,9,56,24,14,19,0,26,-14,19,-56,24),
                     C(190,144,71),C(73,61,52),C(25,26,30),5);
                Ellipse(g,-10,8,20,15,C(32,79,102),C(229,178,87),3);
                // tapered faceted blade with a dark core and polished edges
                Poly(g,P(0,-196,27,-112,33,-2,13,11,-13,11,-33,-2,-27,-112),
                     C(144,173,180),C(49,66,75),C(20,31,38),6);
                Poly(g,P(0,-196,0,5,-25,-2,-23,-108),
                     C(227,239,236),C(86,112,126),C(82,104,111),2);
                Poly(g,P(0,-196,26,-109,29,-4,0,5),
                     C(133,159,170),C(43,63,79),C(66,84,93),2);
                Stroke(g,C(241,247,238),3,-20,-112,-2,-174);
                Stroke(g,C(26,39,47),3,1,-167,2,-12);
                Stroke(g,C(213,174,99),3,-46,12,-14,9);
            }
            bmp.Save(path,ImageFormat.Png);
        }
    }
    public static void Bow(string path) {
        Graphics g; using(Bitmap bmp=Canvas(out g)) {
            using(g) {
                g.TranslateTransform(256,256);g.RotateTransform(-13.0f);
                // dark iron tips and strong but fine pale bowstring
                Stroke(g,C(31,36,41),16,56,-191,76,-172);
                Stroke(g,C(31,36,41),16,56,191,76,172);
                Stroke(g,C(228,213,173),4,56,-191,96,0,56,191);
                // layered yew limbs, drawn as matched handcrafted curves
                using(GraphicsPath wood=new GraphicsPath()) {
                    wood.AddBezier(new PointF(56,-191),new PointF(-75,-119),new PointF(-84,-52),new PointF(-62,0));
                    wood.AddBezier(new PointF(-62,0),new PointF(-84,52),new PointF(-75,119),new PointF(56,191));
                    using(Pen dark=new Pen(C(24,27,28),23)) {dark.LineJoin=LineJoin.Round;g.DrawPath(dark,wood);}
                    using(Pen oak=new Pen(C(116,66,39),17)) {oak.LineJoin=LineJoin.Round;g.DrawPath(oak,wood);}
                    using(Pen light=new Pen(C(175,122,66),5)) g.DrawPath(light,wood);
                }
                // leather reinforced central grip
                Stroke(g,C(29,25,25),23,-62,-31,-66,0,-62,31);
                Stroke(g,C(82,47,37),17,-62,-31,-66,0,-62,31);
                for(int i=-2;i<=2;i++) Stroke(g,C(191,142,81),3,-75,i*10-2,-54,i*10+3);
                // bronze nocks and tiny ornamental studs
                Ellipse(g,49,-201,16,20,C(169,122,55),C(41,38,37),4);
                Ellipse(g,49,181,16,20,C(169,122,55),C(41,38,37),4);
                Ellipse(g,-72,-10,11,11,C(63,121,137),C(218,165,82),3);
                Stroke(g,C(240,227,199),2,56,-188,96,0,56,188);
            }
            bmp.Save(path,ImageFormat.Png);
        }
    }
    public static void Shield(string path) {
        Graphics g; using(Bitmap bmp=Canvas(out g)) {
            using(g) {
                g.TranslateTransform(256,256);
                g.RotateTransform(-15.0f);
                // round shield silhouette and broad embossed steel rim
                Ellipse(g,-192,-192,384,384,C(41,50,52),C(20,23,27),14);
                Ellipse(g,-175,-175,350,350,C(112,83,54),C(191,148,73),18);
                // deep-grain wood segments; color variation does not indicate rarity
                Poly(g,P(-159,-123,-102,-158,-102,156,-159,122),C(128,76,43),C(67,39,30),C(42,34,32),2);
                Poly(g,P(-100,-158,-30,-173,-30,173,-100,158),C(117,61,42),C(66,37,32),C(45,31,27),2);
                Poly(g,P(-28,-172,40,-170,40,170,-28,172),C(140,82,53),C(77,46,34),C(46,31,29),2);
                Poly(g,P(42,-171,105,-153,105,154,42,170),C(108,64,46),C(65,38,30),C(40,29,27),2);
                Poly(g,P(108,-151,157,-123,157,122,108,151),C(134,80,53),C(75,43,36),C(44,31,29),2);
                // engraved horizontal quarter-bars, iron bracing
                Stroke(g,C(56,62,61),13,-151,-80,151,-80);
                Stroke(g,C(107,117,107),3,-148,-85,148,-85);
                Stroke(g,C(56,62,61),13,-151,86,151,86);
                Stroke(g,C(103,111,106),3,-145,81,145,81);
                // boss, crossed brass embellishment and dark heraldic inlays
                Ellipse(g,-76,-76,152,152,C(38,57,63),C(229,188,103),11);
                Poly(g,P(0,-60,12,-12,55,0,12,12,0,60,-12,12,-55,0,-12,-12),
                    C(154,171,164),C(51,69,76),C(27,33,36),4);
                Ellipse(g,-27,-27,54,54,C(76,134,149),C(226,180,92),7);
                Ellipse(g,-10,-10,20,20,C(188,212,203),C(67,90,89),3);
                for (int i=0;i<16;i++) {
                    double a=i*2.0*Math.PI/16.0;
                    float cx=(float)(154*Math.Cos(a)),cy=(float)(154*Math.Sin(a));
                    Ellipse(g,cx-7,cy-7,14,14,C(183,145,84),C(46,40,35),2);
                }
                using(Pen ring=new Pen(C(44,52,57),8))g.DrawEllipse(ring,-181,-181,362,362);
                using(Pen shine=new Pen(C(194,188,147),4))g.DrawArc(shine,-180,-180,360,360,203,160);
            }
            bmp.Save(path,ImageFormat.Png);
        }
    }
}
'@

[ARM0163PilotIcons]::Dagger((Join-Path $Destination 'T_Weapon_Dagger_A.png'))
[ARM0163PilotIcons]::Bow((Join-Path $Destination 'T_Weapon_ShortBow_A.png'))
[ARM0163PilotIcons]::Shield((Join-Path $Destination 'T_Weapon_RoundShield_A.png'))
Get-ChildItem -LiteralPath $Destination -Filter 'T_Weapon_*.png' | ForEach-Object {
    if ($_.Length -lt 2048) { throw "Image incomplete : $($_.FullName)" }
    Write-Host "[ARMEMENT01.6.3] $($_.Name) - $($_.Length) octets"
}
