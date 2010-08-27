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
     start     = 900;
     stop      = 1100;
     step      = 5;
     class     = "Pressurecontourmajor";
     output    = "--PRESSURELINES--";
   },
   {
     parameter = "Pressure";
     start     = 902.5;
     stop      = 1097.5;
     step      = 5;
     class     = "Pressurecontourminor";
     output    = "--PRESSURELINES--";
   }
 );

 graphicsymbol:
 {
     size = 59;
 };

 coldfront:
 {
     letter-spacing = 60;
 };
 warmfront:
 {
     letter-spacing = 60;
 };
 occludedfront:
 {
     letter-spacing = 60;
 };
 trough:
 {
     letter-spacing = 0;
 };
 uppertrough:
 {
     letter-spacing = 0;
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
   fmi-size: 59px;
 }
 .Mirrisymbol
 {
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
   fill: blue;
   font-size: 170px;
   stroke: #eee;
   stroke-width: 2px;
 }
 /* M : low pressure */
 .Mirrisymbol53, Mirrisymbol54, Mirrisymbol55, Mirrisymbol56
 {
   fill: red;
   font-size: 170px;
   stroke: #eee;
   stroke-width: 2px;
 }
 .Temperaturevalue
 {
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
   font-family: Temperaturefont;
   font-size: 70px;
   text-anchor: middle;
   alignment-baseline: middle;
   fill: white;
   stroke: black;
   stroke-width: 1px;
 }
 .Pressurecontourmajor
 {
   fill: none;
   stroke: white;
   stroke-width: 2px;
 }
 .Pressurecontourminor
 {
   fill: none;
   stroke: white;
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
   stroke: #425de8;
   fill: none;
   stroke-width: 5px;
 }
 .coldfrontglyph
 {
   font-family: weatherfont;
   font-size: 30px;
   text-anchor: middle;
   fill: #425de8;
 }
 .warmfront
 {
   stroke: #e42727;
   fill: none;
   stroke-width: 5px;
 }
 .warmfrontglyph
 {
   font-family: weatherfont;
   font-size: 30px;
   text-anchor: middle;
   fill: #e42727;
 }
 .occludedfront
 {
   stroke: #da25d8;
   fill: none;
   stroke-width: 5px;
 }
 .occludedfrontglyph
 {
   font-family: weatherfont;
   font-size: 30px;
   text-anchor: middle;
   fill: #da25d8;
 }
 .trough
 {
   fill: none;
   stroke: #b817e1;
   stroke-width: 5px
 }
 .troughglyph
 {
   font-family: weatherfont;
   fill: #b817e1;
   stroke: none;
   text-anchor: middle;
   font-size: 40px;
   fmi-letter-spacing: 0px;
 }
 .uppertrough
 {
   fill: none;
   stroke: #b817e1;
   stroke-width: 5px;
   stroke-dasharray: 30,10;
   stroke-dashoffset: 0;
 }
 .uppertroughglyph
 {
   font-family: weatherfont;
   fill: #b817e1;
   stroke: none;
   text-anchor: middle;
   font-size: 40px;
   fmi-letter-spacing: 0px;
 }
 .raindots
 {
   fill: url(#waterdotpattern);
   stroke: green;
   stroke-width: 3px;
 }
 .precipitation
 {
   fill: #cff;
   /* fill: url(#precipitationpattern); */
   stroke: none;
 }
 .precipitationmask
 {
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#precipitationfilter);
 }
 .rain
 {
   fill: #cff;
   stroke: none;
 }
 .rainmask
 {
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#rainfilter);
 }
 .snow
 {
   fill: #fff;
   stroke: none;
 }
 .snowmask
 {
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#snowfilter);
 }
 .fog
 {
   display: none;
 }
 .sleet
 {
   fill: #eee;
   stroke: none;
 }
 .sleetmask
 {
   fill: none;
   stroke: white;
   stroke-width: 40px;
   filter: url(#sleetfilter);
 }
 .hail
 {
   display: none;
 }
 .freezing_precipitation
 {
   display: none;
 }
 .drizzle
 {
   display: none;
 }
 .mixed
 {
   display: none;
 }
 .jet
 {
   fill: none;
   stroke: url(#jetgradient);
   stroke-width: 18px;
   marker-end: url(#jetmarker);
 }
 .cloudborder
 {
   display: none;
 }

 .cloudborderglyph
 {
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
--POINTSYMBOLS--
--POINTVALUES--
</g>

</svg>
