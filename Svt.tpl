<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"  width="--WIDTH--px" height="--HEIGHT--px" viewBox="0 0 --WIDTH-- --HEIGHT--">
<title>Weather Fronts</title>

<defs>

 <!-- *** CUSTOMER SPECIFIC FIXED PART STARTS *** -->

 <!-- style sheet -->

 <style type="text/css"><![CDATA[
 @font-face
 {
   font-family: Mirri;
   src: url('http://share.weatherproof.fi/fonts/ttf/Mirri.ttf');
 }
 .Mirrisymbol
 {
   font-family: Mirri;
   font-size: 70px;
   text-anchor: middle;
   fill: yellow;
   stroke: black;
   stroke-width: 1px;
 }
 .Mirrisymbol49 /* K : high pressure */
 {
   fill: blue;
   font-size: 150px;
 }
 .Mirrisymbol53 /* M : low pressure */
 {
   fill: red;
   font-size: 150px;
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
   stroke: blue;
   fill: none;
   stroke-width: 4px;
 }
 .warmfront
 {
   stroke: red;
   fill: none;
   stroke-width: 4px;
 }
 .occludedfront
 {
   stroke: #f0f;
   fill: none;
   stroke-width: 4px;
 }
 .warmfrontglyph
 {
   font-family: weatherfont;
   font-size: 20px;
   text-anchor: middle;
   fill: red;
   fmi-letter-spacing: 60px;
 }
 .coldfrontglyph
 {
   font-family: weatherfont;
   font-size: 20px;
   text-anchor: middle;
   fill: blue;
   fmi-letter-spacing: 60px;
 }
 .occludedfrontglyph
 {
   font-family: weatherfont;
   font-size: 20px;
   text-anchor: middle;
   fill: #f0f;
   fmi-letter-spacing: 60px;
 }
 .trough
 {
   fill: none;
   stroke: #066;
   stroke-width: 5px
 }
 .troughglyph
 {
   font-family: weatherfont;
   fill: #066;
   stroke: none;
   text-anchor: middle;
   font-size: 50px;
   fmi-letter-spacing: 60px;
 }
 .uppertrough
 {
   fill: none;
   stroke: url(#troughgradient);
   stroke-width: 3px;
   stroke-dasharray: 11.1,11.1;
   stroke-dashoffset: 0;
 }
 .uppertroughglyph
 {
   font-family: weatherfont;
   fill: url(#troughgradient);
   stroke: none;
   text-anchor: middle;
   font-size: 50px;
   fmi-letter-spacing: 60px;
 }
 .raindots
 {
   fill: url(#waterdotpattern);
   stroke: green;
   stroke-width: 3px;
 }
 .precipitation
 {
   fill: url(#precipitationpattern);
   stroke: #ccc;
   stroke-width: 2px;
 }
 .heavyprecipitation
 {
   fill: url(#heavyprecipitationpattern);
   stroke: #ccc;
   stroke-width: 3px;
 }
 .precipitationmask
 {
   fill: none;
   stroke: white;
   stroke-width: 15px;
 }
 .heavyprecipitationmask
 {
   fill: none;
   stroke: white;
   stroke-width: 20px;
 }
 .jet
 {
   fill: none;
   stroke: url(#jetgradient);
   stroke-width: 8px;
   marker-end: url(#jetmarker);
 }
 .cloud
 {
   font-family: weatherfont;
   stroke-width: 4px;
   stroke: #999;
   fill: url(#cloudpattern);
   text-anchor: middle;
   font-size: 30px;
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
  <path id="jetmarkerpath" fill="teal" transform="rotate(180) scale(0.6)" d="M 8.7,4.0 L -2.2,0.01 L 8.7,-4.0 C 6.9,-1.6 6.9,1.6 8.7,4.0 z"/>
 </marker> 

 <!-- gradients -->

 <linearGradient id="precipitationgradient">
  <stop offset="5%" stop-color="white"/>
  <stop offset="95%" stop-color="blue"/>
 </linearGradient>

 <linearGradient id="heavyprecipitationgradient">
  <stop offset="5%" stop-color="white"/>
  <stop offset="75%" stop-color="blue"/>
 </linearGradient>

 <linearGradient id="jetgradient">
  <stop offset="0%" stop-color="black"/>
  <stop offset="50%" stop-color="teal"/>
  <stop offset="100%" stop-color="white"/>
 </linearGradient>

 <linearGradient id="troughgradient">
  <stop offset="0%" stop-color="black"/>
  <stop offset="50%" stop-color="teal"/>
  <stop offset="100%" stop-color="white"/>
 </linearGradient>

 <linearGradient id="cloudgradient">
  <stop offset="5%" stop-color="white"/>
  <stop offset="95%" stop-color="blue"/>
 </linearGradient>

 <!-- patterns -->

 <pattern id="waterdotpattern" width="10" height="10" patternUnits="userSpaceOnUse">
  <circle cx="5" cy="5" r="2" fill="url(#precipitationgradient)" fill-opacity="0.5"/>
 </pattern> 

 <pattern id="precipitationpattern" width="6" height="10" patternUnits="userSpaceOnUse">
  <line x1="0" y1="0" x2="6" y2="10" stroke="#ccc"/>
 </pattern> 

 <pattern id="heavyprecipitationpattern" width="10" height="10" patternUnits="userSpaceOnUse">
   <line x1="0" y1="0" x2="10" y2="10" stroke="#ddd"/>
   <circle cx="5" cy="5" r="2" fill="url(#heavyprecipitationgradient)" fill-opacity="0.5"/>
 </pattern> 

 <pattern id="cloudpattern" width="6" height="6" patternUnits="userSpaceOnUse">
  <circle cx="5" cy="5" r="3" fill="url(#cloudgradient)" fill-opacity="0.5"/>
 </pattern> 

 <!-- filters -->

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

<g id="weather" class="weather">
--CLOUDBORDERS--
--PRECIPITATIONAREAS--
--COLDFRONTS--
--OCCLUDEDFRONTS--
--WARMFRONTS--
--TROUGHS--
--UPPERTROUGHS--
--JETS--
--POINTNOTES--
--POINTSYMBOLS--
--POINTVALUES--
</g>
<use class="postweather" xlink:href="#weather"/>

</svg>
