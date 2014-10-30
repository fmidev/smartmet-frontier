<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"  width="--WIDTH--px" height="--HEIGHT--px" viewBox="0 0 --WIDTH-- --HEIGHT--">
<title>Weather Fronts</title>

<defs>

 <!-- *** CUSTOMER SPECIFIC FIXED PART STARTS *** -->

 <!--

 frontier C++ configuration

 <frontier>

 models:
 {
   ECMWF = "/smartmet/data/ecmwf/eurooppa/pinta_xh/querydata/";
 };

 contourlines:
 (
   {
     parameter = "Pressure";
     start     = 900.0;
     stop      = 1100.0;
     step      = 2.5;
     class     = "Pressurecontoursmooth2";
     output    = "--PRESSURELINES--";

     smoother        = "Savitzky-Golay";
     smoother-size   = 5;  // 0...6
     smoother-degree = 2;  // 0...5
   }
 );

 contourlabels:
 (
   {
     parameter = "Pressure";
     modulo    = 1;
     accuracy  = 0.4;
     class     = "PressureLabel";
     symbol    = "PressureBox";
     symbolclass = "PressureBox";
     output    = "--PRESSURELABELS--";

     latlons = [-30.,20., -25.,20., -20.,20., -15.,20., -10.,20., -5.,20., 0.,20., 5.,20., 10.,20., 15.,20., 20.,20., 25.,20., 30.,20., 35.,20., 40.,20., 45.,20., 50.,20., 55.,20., 60.,20., 65.,20., 70.,20., 75.,20., 80.,20., 
-30.,25., -25.,25., -20.,25., -15.,25., -10.,25., -5.,25., 0.,25., 5.,25., 10.,25., 15.,25., 20.,25., 25.,25., 30.,25., 35.,25., 40.,25., 45.,25., 50.,25., 55.,25., 60.,25., 65.,25., 70.,25., 75.,25., 80.,25., 
-30.,30., -25.,30., -20.,30., -15.,30., -10.,30., -5.,30., 0.,30., 5.,30., 10.,30., 15.,30., 20.,30., 25.,30., 30.,30., 35.,30., 40.,30., 45.,30., 50.,30., 55.,30., 60.,30., 65.,30., 70.,30., 75.,30., 80.,30., 
-30.,35., -25.,35., -20.,35., -15.,35., -10.,35., -5.,35., 0.,35., 5.,35., 10.,35., 15.,35., 20.,35., 25.,35., 30.,35., 35.,35., 40.,35., 45.,35., 50.,35., 55.,35., 60.,35., 65.,35., 70.,35., 75.,35., 80.,35., 
-30.,40., -25.,40., -20.,40., -15.,40., -10.,40., -5.,40., 0.,40., 5.,40., 10.,40., 15.,40., 20.,40., 25.,40., 30.,40., 35.,40., 40.,40., 45.,40., 50.,40., 55.,40., 60.,40., 65.,40., 70.,40., 75.,40., 80.,40., 
-30.,45., -25.,45., -20.,45., -15.,45., -10.,45., -5.,45., 0.,45., 5.,45., 10.,45., 15.,45., 20.,45., 25.,45., 30.,45., 35.,45., 40.,45., 45.,45., 50.,45., 55.,45., 60.,45., 65.,45., 70.,45., 75.,45., 80.,45., 
-30.,50., -25.,50., -20.,50., -15.,50., -10.,50., -5.,50., 0.,50., 5.,50., 10.,50., 15.,50., 20.,50., 25.,50., 30.,50., 35.,50., 40.,50., 45.,50., 50.,50., 55.,50., 60.,50., 65.,50., 70.,50., 75.,50., 80.,50., 
-30.,55., -25.,55., -20.,55., -15.,55., -10.,55., -5.,55., 0.,55., 5.,55., 10.,55., 15.,55., 20.,55., 25.,55., 30.,55., 35.,55., 40.,55., 45.,55., 50.,55., 55.,55., 60.,55., 65.,55., 70.,55., 75.,55., 80.,55., 
-30.,60., -25.,60., -20.,60., -15.,60., -10.,60., -5.,60., 0.,60., 5.,60., 10.,60., 15.,60., 20.,60., 25.,60., 30.,60., 35.,60., 40.,60., 45.,60., 50.,60., 55.,60., 60.,60., 65.,60., 70.,60., 75.,60., 80.,60., 
-30.,65., -25.,65., -20.,65., -15.,65., -10.,65., -5.,65., 0.,65., 5.,65., 10.,65., 15.,65., 20.,65., 25.,65., 30.,65., 35.,65., 40.,65., 45.,65., 50.,65., 55.,65., 60.,65., 65.,65., 70.,65., 75.,65., 80.,65., 
-30.,70., -25.,70., -20.,70., -15.,70., -10.,70., -5.,70., 0.,70., 5.,70., 10.,70., 15.,70., 20.,70., 25.,70., 30.,70., 35.,70., 40.,70., 45.,70., 50.,70., 55.,70., 60.,70., 65.,70., 70.,70., 75.,70., 80.,70., 
-30.,75., -25.,75., -20.,75., -15.,75., -10.,75., -5.,75., 0.,75., 5.,75., 10.,75., 15.,75., 20.,75., 25.,75., 30.,75., 35.,75., 40.,75., 45.,75., 50.,75., 55.,75., 60.,75., 65.,75., 70.,75., 75.,75., 80.,75., 
-30.,80., -25.,80., -20.,80., -15.,80., -10.,80., -5.,80., 0.,80., 5.,80., 10.,80., 15.,80., 20.,80., 25.,80., 30.,80., 35.,80., 40.,80., 45.,80., 50.,80., 55.,80., 60.,80., 65.,80., 70.,80., 75.,80., 80.,80.];

   }
 );

 graphicsymbol:
 {
     size = 59.0;
 };

 coldfront:
 {
     letter-spacing = 60.0;
 };
 warmfront:
 {
     letter-spacing = 60.0;
 };
 occludedfront:
 {
     letter-spacing = 60.0;
 };
 trough:
 {
     letter-spacing = 0.0;
 };
 uppertrough:
 {
     letter-spacing = 0.0;
 };
 </frontier>

 -->

 <!-- style sheet -->

 <style type="text/css"><![CDATA[
 @font-face
 {
   font-family: Mirri;
   src: url('http://share.weatherproof.fi/fonts/ttf/Mirri.ttf');
 }
 @font-face
 {
   font-family: Temperaturefont;
   src: url('http://share.weatherproof.fi/fonts/ttf/Frutiger-Black.ttf');
 }
 .graphicsymbol
 {
   display: none;
   fmi-size: 59px;
 }
 .PressureLabel
 {
     font-family: Arial;
     font-size: 13px;
     text-anchor: middle;
 }
 .PressureBox
 {
     stroke-width: 0.2px;
     stroke: black;
     fill: white;
 }
 .Mirrisymbol
 {
   display: none;
   font-family: Mirri;
   font-size: 110px;
   text-anchor: middle;
   fill: #8b0;
   stroke: black;
   stroke-width: 1px;
 }
 /* K : high pressure */
 .Mirrisymbol49, Mirrisymbol50, Mirrisymbol51, Mirrisymbol52
 {
   display: none;
   fill: blue;
   font-size: 170px;
   stroke: #eee;
   stroke-width: 2px;
 }
 /* M : low pressure */
 .Mirrisymbol53, Mirrisymbol54, Mirrisymbol55, Mirrisymbol56
 {
   display: none;
   fill: red;
   font-size: 170px;
   stroke: #eee;
   stroke-width: 2px;
 }
 .Temperaturevalue
 {
   display: none;
   font-family: Temperaturefont;
   font-size: 70px;
   text-anchor: middle;
   alignment-baseline: middle;
   fill: white;
   stroke: black;
   stroke-width: 1px;
 }
 .Temperaturerange
 {
   display: none;
   font-family: Temperaturefont;
   font-size: 70px;
   text-anchor: middle;
   alignment-baseline: middle;
   fill: white;
   stroke: black;
   stroke-width: 1px;
 }
 .Pressurecontour
 {
   fill: none;
   stroke: white;
   stroke-width: 2px;
 }
 .Pressurecontoursmooth
 {
   fill: none;
   stroke: red;
   stroke-width: 1px;
 }
 .Pressurecontoursmooth2
 {
   fill: none;
   stroke: cyan;
   stroke-width: 1px;
 }
 .Pressurecontoursmooth3
 {
   fill: none;
   stroke: yellow;
   stroke-width: 1px;
 }

 .weather
 {
   filter: url(#shadow);
 }
 .postweather
 {
   filter: url(#emboss);
 }
 .coldfront
 {
   display: none;
   stroke: #425de8;
   fill: none;
   stroke-width: 5px;
 }
 .coldfrontglyph
 {
   display: none;
   font-family: weatherfont;
   font-size: 30px;
   text-anchor: middle;
   fill: #425de8;
 }
 .warmfront
 {
   display: none;
   stroke: #e42727;
   fill: none;
   stroke-width: 5px;
 }
 .warmfrontglyph
 {
   display: none;
   font-family: weatherfont;
   font-size: 30px;
   text-anchor: middle;
   fill: #e42727;
 }
 .occludedfront
 {
   display: none;
   stroke: #da25d8;
   fill: none;
   stroke-width: 5px;
 }
 .occludedfrontglyph
 {
   display: none;
   font-family: weatherfont;
   font-size: 30px;
   text-anchor: middle;
   fill: #da25d8;
 }
 .trough
 {
   display: none;
   fill: none;
   stroke: #b817e1;
   stroke-width: 5px
 }
 .troughglyph
 {
   display: none;
   font-family: weatherfont;
   fill: #b817e1;
   stroke: none;
   text-anchor: middle;
   font-size: 40px;
   fmi-letter-spacing: 0px;
 }
 .uppertrough
 {
   display: none;
   fill: none;
   stroke: #b817e1;
   stroke-width: 5px;
   stroke-dasharray: 30,10;
   stroke-dashoffset: 0;
 }
 .uppertroughglyph
 {
   display: none;
   font-family: weatherfont;
   fill: #b817e1;
   stroke: none;
   text-anchor: middle;
   font-size: 40px;
   fmi-letter-spacing: 0px;
 }
 .raindots
 {
   display: none;
   fill: url(#waterdotpattern);
   stroke: green;
   stroke-width: 3px;
 }
 .precipitation
 {
   display: none;
   fill: #cff;
   /* fill: url(#precipitationpattern); */
   stroke: none;
 }
 .precipitationmask
 {
   display: none;
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#precipitationfilter);
 }
 .rain
 {
   display: none;
   fill: #cff;
   stroke: none;
 }
 .rainmask
 {
   display: none;
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#rainfilter);
 }
 .snow
 {
   display: none;
   fill: #fff;
   stroke: none;
 }
 .snowmask
 {
   display: none;
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#snowfilter);
 }
 .fog
 {
   display: none;
   display: none;
 }
 .sleet
 {
   display: none;
   fill: #eee;
   stroke: none;
 }
 .sleetmask
 {
   display: none;
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#sleetfilter);
 }
 .hail
 {
   display: none;
   display: none;
 }
 .freezing_precipitation
 {
   display: none;
   display: none;
 }
 .drizzle
 {
   display: none;
   display: none;
 }
 .mixed
 {
   display: none;
   display: none;
 }
 .jet
 {
   display: none;
   fill: none;
   stroke: url(#jetgradient);
   stroke-width: 18px;
   marker-end: url(#jetmarker);
 }
 .cloudborder
 {
   display: none;
   display: none;
 }

 .cloudborderglyph
 {
   display: none;
   font-family: weatherfont;
   stroke-width: 1.75px;
   stroke: #bbb;
   fill: none;
   text-anchor: middle;
   font-size: 35px;
 }

 ]]></style>

 <!-- weather font -->

 <font>
  <font-face font-family="weatherfont" units-per-em="100" alphabetic="0"/>
  <missing-glyph horiz-adv-x="100" d="M0,0 L0,0 0,50 50,50z"/>

  <!-- warm front -->
  <glyph unicode="w" horiz-adv-x="100" d="M0,0 A 50,-50 0 0 0 100,0z"/>
  <glyph unicode="W" horiz-adv-x="100" d="M0,0 A 50,50 0 0 0 100,0z"/>

  <!-- cold front -->
  <glyph unicode="C" horiz-adv-x="100" d="M0,0 L 50,80 100,0z"/>
  <glyph unicode="c" horiz-adv-x="100" d="M0,0 L 50,-80 100,0z"/>

  <!-- trough -->
  <glyph unicode="t" horiz-adv-x="100" d="M0,0 L 80,50 90,45 25,0z"/>
  <glyph unicode="T" horiz-adv-x="100" d="M0,0 L 80,-50 90,-45 25,0z"/>

  <!-- clouds -->
  <glyph unicode="U" horiz-adv-x="100" d="M0,0 A 50,50 0 0 0 100,0"/>

 </font>

 <!-- markers -->

 <marker orient="auto" id="jetmarker" style="overflow:visible;">
  <path id="jetmarkerpath" fill="crimson" d="M -2.3,-2.0 C -1.2,-0.8 -1.2 0.8 -2.3 2.0 L 1.1,0 z"/>
 </marker> 

 <!-- gradients -->

 <linearGradient id="precipitationgradient">
  <stop offset="5%" stop-color="white"/>
  <stop offset="95%" stop-color="blue"/>
 </linearGradient>

 <linearGradient id="jetgradient">
  <stop offset="0%" stop-color="crimson" stop-opacity="0"/>
  <stop offset="40%" stop-color="crimson" stop-opacity="0.8"/>
  <stop offset="100%" stop-color="crimson" stop-opacity="0.8"/>
 </linearGradient>

<symbol id="PressureBox">
 <rect width="34px" height="20px" x="-17px" y="-14px"/>
</symbol>

 <!-- patterns -->

 <pattern id="precipitationpattern" width="6" height="10" patternUnits="userSpaceOnUse">
  <line x1="0" y1="0" x2="6" y2="10" stroke="#eee"/>
 </pattern> 

 <!-- filters -->

 <filter id="precipitationfilter" width="150%" height="150%">
  <feGaussianBlur in="SourceGraphic" stdDeviation="20"/>
 </filter>

 <filter id="rainfilter" width="150%" height="150%">
  <feGaussianBlur in="SourceGraphic" stdDeviation="20"/>
 </filter>

 <filter id="snowfilter" width="150%" height="150%">
  <feGaussianBlur in="SourceGraphic" stdDeviation="20"/>
 </filter>

 <filter id="sleetfilter" width="150%" height="150%">
  <feGaussianBlur in="SourceGraphic" stdDeviation="20"/>
 </filter>

 <filter id="shadow" width="150%" height="150%">
  <feOffset in="SourceAlpha" result="offsetted" dx="3" dy="5"/>
  <feGaussianBlur in="offsetted" result="blurred" stdDeviation="4"/>
  <feBlend in="SourceGraphic" in2="blurred" mode="normal"/>
 </filter>

 <filter id="emboss">
  <feGaussianBlur in="SourceAlpha" result="blurred" stdDeviation="2"/>
  <feSpecularLighting in="blurred" result="lighted" surfaceScale="-3" style="lighting-color:white" specularConstant="1" specularExponent="16" kernelUnitLength="1" >
   <feDistantLight azimuth="45" elevation="45"/>
  </feSpecularLighting>
  <feComposite in="lighted" in2="SourceGraphic" operator="in"/>
 </filter>

 <!-- *** CUSTOMER SPECIFIC FIXED PART ENDS *** -->

 <!-- *** actual weather paths from WOML data *** -->

 --PATHS--

 <!-- *** masks for all rain objects *** -->

 --MASKS--

</defs>

<!-- *** ACTUAL GRAPH OBJECTS START -->


<!-- weather begins -->

<g id="contourlines">
 --PRESSURELINES--
 --PRESSURELABELS--
</g>

<g id="weather1" class="weather">
--CLOUDBORDERS--
--PRECIPITATIONAREAS--
</g>
<g id="weather2" class="weather">
--COLDFRONTS--
--OCCLUDEDFRONTS--
--WARMFRONTS--
--TROUGHS--
--UPPERTROUGHS--
</g>
<use class="postweather" xlink:href="#weather2"/>
<g id="weather3" class="weather">
--JETS--
</g>
<use class="postweather" xlink:href="#weather3"/>
<g id="weather4" class="weather">
--POINTNOTES--
--POINTVALUES--
</g>

</svg>
