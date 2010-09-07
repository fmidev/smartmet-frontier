<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"  width="--WIDTH--px" height="--HEIGHT--px" viewBox="0 0 --WIDTH-- --HEIGHT--">
<title>Weather Fronts</title>

<defs>

 <!-- *** CUSTOMER SPECIFIC FIXED PART STARTS *** -->

 <!--

 frontier C++ configuration

 <frontier>

 contourlines:
 (
   {
     parameter = "Pressure";
     start     = 900.0;
     stop      = 1100.0;
     step      = 2.5;
     class     = "Pressurecontour";
     output    = "--PRESSURELINES--";
     smoother  = "none";
   },
   {
     parameter = "Pressure";
     start     = 900.0;
     stop      = 1100.0;
     step      = 2.5;
     class     = "Pressurecontoursmooth";
     output    = "--PRESSURELINES--";

     smoother        = "Savitzky-Golay";
     smoother-size   = 4;  // 0...6
     smoother-degree = 2;  // 0...5
   },
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
   },
   {
     parameter = "Pressure";
     start     = 900.0;
     stop      = 1100.0;
     step      = 2.5;
     class     = "Pressurecontoursmooth3";
     output    = "--PRESSURELINES--";

     smoother        = "Savitzky-Golay";
     smoother-size   = 6;  // 0...6
     smoother-degree = 2;  // 0...5
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

<image id="background" xlink:href="Svt_pohja.png" width="1920px" height="1080px"/>

<!-- weather begins -->

<g id="contourlines">
 --PRESSURELINES--
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
