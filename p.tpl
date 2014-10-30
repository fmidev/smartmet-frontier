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

     labels:
     {
          modulo      = 5;
	  class       = "PressureLabel";
	  symbol      = "PressureBox";
     	  symbolclass = "PressureBox";
     	  output      = "--PRESSURELABELS--";
	  mindistance = 5.0;

      coordinates = [
20,20, 70,20, 120,20, 170,20, 220,20, 270,20, 320,20, 370,20, 420,20, 470,20, 520,20, 570,20, 620,20, 670,20, 720,20, 770,20, 820,20, 870,20, 920,20, 970,20, 1020,20, 1070,20, 1120,20, 1170,20, 1220,20, 1270,20, 1320,20, 1370,20, 1420,20, 1470,20, 
45,70, 95,70, 145,70, 195,70, 245,70, 295,70, 345,70, 395,70, 445,70, 495,70, 545,70, 595,70, 645,70, 695,70, 745,70, 795,70, 845,70, 895,70, 945,70, 995,70, 1045,70, 1095,70, 1145,70, 1195,70, 1245,70, 1295,70, 1345,70, 1395,70, 1445,70, 1495,70, 
20,120, 70,120, 120,120, 170,120, 220,120, 270,120, 320,120, 370,120, 420,120, 470,120, 520,120, 570,120, 620,120, 670,120, 720,120, 770,120, 820,120, 870,120, 920,120, 970,120, 1020,120, 1070,120, 1120,120, 1170,120, 1220,120, 1270,120, 1320,120, 1370,120, 1420,120, 1470,120, 
45,170, 95,170, 145,170, 195,170, 245,170, 295,170, 345,170, 395,170, 445,170, 495,170, 545,170, 595,170, 645,170, 695,170, 745,170, 795,170, 845,170, 895,170, 945,170, 995,170, 1045,170, 1095,170, 1145,170, 1195,170, 1245,170, 1295,170, 1345,170, 1395,170, 1445,170, 1495,170, 
20,220, 70,220, 120,220, 170,220, 220,220, 270,220, 320,220, 370,220, 420,220, 470,220, 520,220, 570,220, 620,220, 670,220, 720,220, 770,220, 820,220, 870,220, 920,220, 970,220, 1020,220, 1070,220, 1120,220, 1170,220, 1220,220, 1270,220, 1320,220, 1370,220, 1420,220, 1470,220, 
45,270, 95,270, 145,270, 195,270, 245,270, 295,270, 345,270, 395,270, 445,270, 495,270, 545,270, 595,270, 645,270, 695,270, 745,270, 795,270, 845,270, 895,270, 945,270, 995,270, 1045,270, 1095,270, 1145,270, 1195,270, 1245,270, 1295,270, 1345,270, 1395,270, 1445,270, 1495,270, 
20,320, 70,320, 120,320, 170,320, 220,320, 270,320, 320,320, 370,320, 420,320, 470,320, 520,320, 570,320, 620,320, 670,320, 720,320, 770,320, 820,320, 870,320, 920,320, 970,320, 1020,320, 1070,320, 1120,320, 1170,320, 1220,320, 1270,320, 1320,320, 1370,320, 1420,320, 1470,320, 
45,370, 95,370, 145,370, 195,370, 245,370, 295,370, 345,370, 395,370, 445,370, 495,370, 545,370, 595,370, 645,370, 695,370, 745,370, 795,370, 845,370, 895,370, 945,370, 995,370, 1045,370, 1095,370, 1145,370, 1195,370, 1245,370, 1295,370, 1345,370, 1395,370, 1445,370, 1495,370, 
20,420, 70,420, 120,420, 170,420, 220,420, 270,420, 320,420, 370,420, 420,420, 470,420, 520,420, 570,420, 620,420, 670,420, 720,420, 770,420, 820,420, 870,420, 920,420, 970,420, 1020,420, 1070,420, 1120,420, 1170,420, 1220,420, 1270,420, 1320,420, 1370,420, 1420,420, 1470,420, 
45,470, 95,470, 145,470, 195,470, 245,470, 295,470, 345,470, 395,470, 445,470, 495,470, 545,470, 595,470, 645,470, 695,470, 745,470, 795,470, 845,470, 895,470, 945,470, 995,470, 1045,470, 1095,470, 1145,470, 1195,470, 1245,470, 1295,470, 1345,470, 1395,470, 1445,470, 1495,470, 
20,520, 70,520, 120,520, 170,520, 220,520, 270,520, 320,520, 370,520, 420,520, 470,520, 520,520, 570,520, 620,520, 670,520, 720,520, 770,520, 820,520, 870,520, 920,520, 970,520, 1020,520, 1070,520, 1120,520, 1170,520, 1220,520, 1270,520, 1320,520, 1370,520, 1420,520, 1470,520, 
45,570, 95,570, 145,570, 195,570, 245,570, 295,570, 345,570, 395,570, 445,570, 495,570, 545,570, 595,570, 645,570, 695,570, 745,570, 795,570, 845,570, 895,570, 945,570, 995,570, 1045,570, 1095,570, 1145,570, 1195,570, 1245,570, 1295,570, 1345,570, 1395,570, 1445,570, 1495,570, 
20,620, 70,620, 120,620, 170,620, 220,620, 270,620, 320,620, 370,620, 420,620, 470,620, 520,620, 570,620, 620,620, 670,620, 720,620, 770,620, 820,620, 870,620, 920,620, 970,620, 1020,620, 1070,620, 1120,620, 1170,620, 1220,620, 1270,620, 1320,620, 1370,620, 1420,620, 1470,620, 
45,670, 95,670, 145,670, 195,670, 245,670, 295,670, 345,670, 395,670, 445,670, 495,670, 545,670, 595,670, 645,670, 695,670, 745,670, 795,670, 845,670, 895,670, 945,670, 995,670, 1045,670, 1095,670, 1145,670, 1195,670, 1245,670, 1295,670, 1345,670, 1395,670, 1445,670, 1495,670, 
20,720, 70,720, 120,720, 170,720, 220,720, 270,720, 320,720, 370,720, 420,720, 470,720, 520,720, 570,720, 620,720, 670,720, 720,720, 770,720, 820,720, 870,720, 920,720, 970,720, 1020,720, 1070,720, 1120,720, 1170,720, 1220,720, 1270,720, 1320,720, 1370,720, 1420,720, 1470,720, 
45,770, 95,770, 145,770, 195,770, 245,770, 295,770, 345,770, 395,770, 445,770, 495,770, 545,770, 595,770, 645,770, 695,770, 745,770, 795,770, 845,770, 895,770, 945,770, 995,770, 1045,770, 1095,770, 1145,770, 1195,770, 1245,770, 1295,770, 1345,770, 1395,770, 1445,770, 1495,770, 
20,820, 70,820, 120,820, 170,820, 220,820, 270,820, 320,820, 370,820, 420,820, 470,820, 520,820, 570,820, 620,820, 670,820, 720,820, 770,820, 820,820, 870,820, 920,820, 970,820, 1020,820, 1070,820, 1120,820, 1170,820, 1220,820, 1270,820, 1320,820, 1370,820, 1420,820, 1470,820, 
45,870, 95,870, 145,870, 195,870, 245,870, 295,870, 345,870, 395,870, 445,870, 495,870, 545,870, 595,870, 645,870, 695,870, 745,870, 795,870, 845,870, 895,870, 945,870, 995,870, 1045,870, 1095,870, 1145,870, 1195,870, 1245,870, 1295,870, 1345,870, 1395,870, 1445,870, 1495,870, 
20,920, 70,920, 120,920, 170,920, 220,920, 270,920, 320,920, 370,920, 420,920, 470,920, 520,920, 570,920, 620,920, 670,920, 720,920, 770,920, 820,920, 870,920, 920,920, 970,920, 1020,920, 1070,920, 1120,920, 1170,920, 1220,920, 1270,920, 1320,920, 1370,920, 1420,920, 1470,920, 
45,970, 95,970, 145,970, 195,970, 245,970, 295,970, 345,970, 395,970, 445,970, 495,970, 545,970, 595,970, 645,970, 695,970, 745,970, 795,970, 845,970, 895,970, 945,970, 995,970, 1045,970, 1095,970, 1145,970, 1195,970, 1245,970, 1295,970, 1345,970, 1395,970, 1445,970, 1495,970];
       };
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
</g>
<g id="contourlabels">
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
