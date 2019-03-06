<html>

Multitexture and the Quake 3 graphics engine



by Bryan McNett

This article will attempt to explain one of Quake 3's most important 
graphics technologies. Afterwards, you may download a file which lets 
you play with Quake 3 technology in Adobe Photoshop.


When cheap 3D hardware acceleration began to proliferate, computer 
game designers who don't understand graphics technology breathed a sigh 
of relief. "Finally," they said, "all games will look equally good, and 
then gameplay will rule the marketplace." In May, 1998, these words bear
 a whiff of truth: games without new graphics technology regularly blow 
people away simply by supporting 3DFX. This will change utterly upon the
 release of id software's "Quake 3", the first computer game engine 
known to grudgingly accept 3DFX as a minimum requirement.

_Light mapping_ is one Quake technology that changed the 
computer game industry forever. Without the detailed shadows made 
possible by light mapping, it is very difficult to convey a sense of 
depth or realism, especially when texture maps repeat. Light mapping is 
also an early example of _multitexture_ in computer games. 
Multitexture refers to the act of mixing two or more texture maps to 
create a new texture map. Multitexture is also the key to the Quake 3 
graphics technology I will discuss. Let¡¯s use light mapping to acquaint 
ourselves with multitexture:

A light map is _multiplied_ with a texture map to produce shadows. 
It may not be clear what this means. At these times, it helps to 
think of colors as numbers between zero and one, with black having 
the value zero and white having the value one. Because white has the 
value one, multiplying it with any other color does not change the 
color:

<center><table bgcolor="#f0f0f0">
<tbody><tr><td align="center">![](https://github.com/suijingfeng/engine/blob/master/doc/Multitexture/smaller.jpg)</td><td align="center">X</td><td align="center">![](https://github.com/suijingfeng/engine/blob/master/doc/Multitexture/white.jpg)</td><td align="center">=</td><td align="center">![](https://github.com/suijingfeng/engine/blob/master/doc/Multitexture/smaller.jpg)</td></tr>
<tr><td align="center">whatever</td><td align="center">times</td><td align="center">one</td><td align="center">equals</td><td align="center">whatever</td></tr>
</tbody></table></center>

Because black has the value zero, multiplying it with any other color changes the color to black:

<center><table bgcolor="#f0f0f0">
<tbody><tr><td align="center">![](Multitexture/smaller.jpg)</td><td align="center">X</td><td align="center">![](Multitexture/black.jpg)</td><td align="center">=</td><td align="center">![](Multitexture/black.jpg)</td></tr>
<tr><td align="center">whatever</td><td align="center">times</td><td align="center">zero</td><td align="center">equals</td><td align="center">zero</td></tr>
</tbody></table></center>

In a light map, shadows are black and the rest is bright. Therefore, 
when the light map is multiplied with the texture map, shadows become 
black and the rest stays essentially the same:

<center><table bgcolor="#f0f0f0">
<tbody><tr><td align="center">![](Multitexture/texturemap.jpg)</td><td align="center">X</td><td align="center">![](Multitexture/lightmap.jpg)</td><td align="center">=</td><td align="center">![](Multitexture/multiplied.jpg)</td></tr>
<tr><td align="center">texture map</td><td align="center">times</td><td align="center">light map</td><td align="center">equals</td><td align="center">shadows</td></tr>
</tbody></table></center>

Let¡¯s look at what happens to one pixel when a light map is _multiplied_
 with a texture map. In this example, I will use pseudocode because it 
is familiar to programmers. P stands for "pixel color", T for "texture 
color", and L for "light map color":
</font><dir><font size="2">
</font><dir><font size="2">

</font><pre>P.r = L.r * T.r; // red
P.g = L.g * T.g; // green
P.b = L.b * T.b; // blue</pre></dir>
</dir>

<font size="2">

To find the pixel color, we _multiply_ the values of the texture map and light map. Since we do this once for each pixel, we may say that we are _multiplying_ the texture maps themselves. 

At this point, we should abandon pseudocode in favor of something with the same meaning ¨C but a more compact form: 
</font><dir><font size="2">
</font><dir><font size="2">

</font>

![](Multitexture/Image1.gif)
</dir>
</dir>

<font size="2">

Those who have studied mathematics should know that this is an equation. In English, this equation is saying: 
</font><dir><font size="2">
</font><dir><font size="2">

</font><font size="2" face="Courier New">

"the pixel color is equal to the texture map multiplied by the light map."
</font></dir><font size="2" face="Courier New">
</font></dir><font size="2" face="Courier New">

</font><font size="2">

Surprisingly, the corresponding equation for 
Quake 3 was announced in public by Brian Hook of id software in May, 
1998. Here is an interpretation:
</font><dir><font size="2">
</font><dir><font size="2">

</font>

![](Multitexture/Image3.gif)
</dir>
</dir>

<font size="2">

Aside from new terms B, S, and ?, Quake 3's equation 
is the same as Quake's. Before we examine the meaning of the new terms, 
however, let¡¯s express this equation as pseudocode, which some people 
find easier to read:
</font><dir><font size="2">
</font><dir><font size="2">

</font><pre>p = L;
for( i=0; i&lt;n; i++ ) {
 p += B[i];
}
p = p * T + S;</pre></dir>
</dir>

<font size="2">

Now let's express it as English:
</font><dir><font size="2">
</font><dir><font size="2">

</font><font size="2" face="Courier New">

"The pixel color is equal to
 the static light map plus the dynamic bump maps, multiplied by the 
texture map, and then added to the specularity map. We aren't sure what 
comes next."
</font></dir><font size="2" face="Courier New">
</font></dir><font size="2" face="Courier New">

</font><font size="2">

I must emphasize that even Mr. Hook admits that
 the Quake 3 engine is subject to change. Also, it is possible that only
 walls get static light maps, or only objects get dynamic bump maps... 
since I'm not involved with Quake 3 development, I have no way of 
knowing the details. With that in mind, let's continue.

We¡¯ve seen texture multiplication before, but here we are introduced to texture _addition_. Where multiplication tends to make textures darker, addition tends to make them brighter:

<center><table bgcolor="#f0f0f0">
<tbody><tr><td align="center">![](Multitexture/smaller.jpg)</td><td align="center">+</td><td align="center">![](Multitexture/white.jpg)</td><td align="center">=</td><td align="center">![](Multitexture/white.jpg)</td></tr>
<tr><td align="center">whatever</td><td align="center">plus</td><td align="center">one</td><td align="center">equals</td><td align="center">one</td></tr>
</tbody></table></center>

White is the brightest color. No matter what you add to it, it just doesn't get any brighter.

<center><table bgcolor="#f0f0f0">
<tbody><tr><td align="center">![](Multitexture/smaller.jpg)</td><td align="center">+</td><td align="center">![](Multitexture/black.jpg)</td><td align="center">=</td><td align="center">![](Multitexture/smaller.jpg)</td></tr>
<tr><td align="center">whatever</td><td align="center">plus</td><td align="center">zero</td><td align="center">equals</td><td align="center">whatever</td></tr>
</tbody></table></center>
<center><table bgcolor="#f0f0f0">
<tbody><tr><td align="center">![](Multitexture/smaller.jpg)</td><td align="center">+</td><td align="center">![](Multitexture/lightmap.jpg)</td><td align="center">=</td><td align="center">![](Multitexture/glare.jpg)</td></tr>
<tr><td align="center">texture map</td><td align="center">plus</td><td align="center">specularity map</td><td align="center">equals</td><td align="center">shiny</td></tr>
</tbody></table></center>

Now to explain the terms B, S, and ? in Quake 3's equation. B stands for _bump map_, though it is <font color="#ff0000">nothing nothing nothing</font> like the _bump maps_ that appear in the computer graphics literature. Quake 3 bump maps are totally new. It is _very_ important that you learn this distinction _now_. The white parts are <font color="#ff0000">not</font>
 the parts that "stick out". Each bump map is the light map for the 
polygon as dynamically lit from a particular range of angles. When all 
the _bump maps_ are _added_, the surface is dynamically lit 
from all angles. As such, Quake 3 bump maps are similar to the light 
maps that appear in Paul Haeberli's article [Synthetic Lighting for Photography](http://www.sgi.com/grafica/synth/index.html).
</font>
<center><table width="638" cellspacing="0" cellpadding="7" border="0">
<tbody><tr><td valign="TOP">

![](Multitexture/bump5.gif)

<font size="2"></font>

<font size="2">A complete light map of a raised button</font>
</td>
</tr>
</tbody></table>
</center>

<center><table width="638" cellspacing="0" cellpadding="7" border="0">
<tbody><tr><td width="50%" valign="TOP" height="11">

![](Multitexture/bump1.gif)

<font size="2"></font>

<font size="2">bump map for when the light is to the upper-left</font>
</td>
<td width="50%" valign="TOP" height="11">

![](Multitexture/bump2.gif)

<font size="2"></font>

<font size="2">bump map for when the light is to the upper-right</font>
</td>
</tr>
<tr><td width="50%" valign="TOP" height="25">

![](Multitexture/bump3.gif)

<font size="2"></font>

<font size="2">bump map for when the light is below</font>
</td>
<td width="50%" valign="TOP" height="25">

![](Multitexture/bump4.gif)

<font size="2"></font>

<font size="2">bump map for when the light is directly in front</font>
</td>
</tr>
</tbody></table>
</center>

<font size="2">

Quake 3¡¯s _bump maps_ are different from ordinary
 light maps because it is possible to modulate their brightness 
independently by adjusting per-vertex iterated or constant color. This 
results in a surface with texel-sized cracks and bumps that are always 
lit from the correct direction. As dynamic lights swing around in real 
time, texel-sized highlights on the cracks and bumps swing around in 
response. The result is a shockingly real environment for those who are 
just now becoming jaded with the visuals in Quake 2.

S stands for _specularity map_. This is a supplementary light map for only the shiny parts of a surface. Where the specularity map is bright, a glare will be _added_ when a light points directly at it. Where it is dark, there is no glare. Because it is a texture map, the _specularity map_
 enables the distinction between shiny and dull parts of a surface to be
 the size of a single texel. Likewise, it is not a choice of fully dull 
or fully shiny ¨C there should be hundreds of shine levels for a texel to
 choose from.

? stands for two or perhaps more additional effects about which I understand little at this time.

<center><font size="5">**Time to Download the Nummy Goodies**</font></center>

For those of you lucky enough to own Photoshop, but somehow not lucky
 enough to work at id software, I've put together a file that lets you 
play with Quake 3's multitexture technology in real time. By playing 
with this file, you can get an early glimpse at the fundamentals of 
Quake 3 texture editing. Unfortunately, like some 3D cards, Photoshop 
does not really support all the required _mixing modes_. This can be fixed by tripling the number of "passes", but I'll leave that as an exercise for the reader.

Once you load _shader.psd_ into Photoshop, open the "layers" window. This should appear:

<center>![](Multitexture/layers.jpg)</center>

Each Photoshop layer corresponds to a single "pass" of multitexture. 
Let's explain how these layers help you to play the roles of "id 
software texture designer", "Quake 3 engine" and "light map compiler".

<center>![](Multitexture/light.jpg)</center>

When something bright appears while you're playing Quake and Quake 2,
 the graphics engine draws bright circles into the light map. The final 
step of Quake level design is the light compiler, another program that 
draws stuff into the light map. You can pretend that you are either the 
light map compiler or the Quake engine by selecting the "light map" 
layer, then doodling into it with the usual Photoshop tools.

<center>![](Multitexture/opacity.jpg)</center>
<center>![](Multitexture/bump.jpg)</center>

The pure white square to the left of the _layer mask_ is like one big white polygon. By doodling into the white square, you're saying _"instead of one big white polygon, there are thousands of colored polygons, each the size of a single pixel."_ Quake 3 will not generally draw polygons that small.

By doodling into the _layer mask_ of bump map #1, however, you are saying _"the
 white parts of this bump map are the parts of the surface that point 
directly away from the surface, like the wall itself and the top of this
 raised button. The black parts are those that do not point directly 
away from the surface, like the edge of the raised button. The gray 
parts are in between."_ By doodling into bump map #2, you are saying _"the white parts are the parts of the surface that point downward, like the bottom edge of this raised button."_ And so on. This is the job of the texture designer.

By adjusting the opacity slider of bump map #1, you are saying _"this much dynamic light is striking the surface directly."_ By adjusting the opacity slider for bump map #2, you are saying _"this much dynamic light is striking the surface from below."_ And so on. This is the job of the Quake 3 graphics engine.

The image will look a billion times better if you make very many tiny
 bumps - not one big bump like this one. The bump maps that ship with 
Quake 3 will probably have lots of tiny bumps and cracks.

<center>![](Multitexture/diffuse.jpg)</center>

The Quake engines don't draw stuff into the diffuse map while you're 
playing. You can play texture designer, however, by doodling on the 
diffuse map layer.

<center>![](Multitexture/opacity.jpg)</center>
<center>![](Multitexture/specularity.jpg)</center>

The pure white square to the left of the _layer mask_ is like one huge white polygon. By doodling into the white square, you're saying _"instead of one big white polygon, there are thousands of colored polygons, each the size of a single pixel."_ Quake 3 will not generally draw polygons that small.

By doodling into the _layer mask_, however, you are saying _"the
 white parts of this specularity map are the parts of the surface that 
are very shiny. The black parts are the parts that are very dull. The 
gray parts are in between."_ This is the job of the texture designer.

By setting the opacity slider to 100%, you are saying _"the dynamic light striking this surface points directly at the surface."_ By setting it to 0%, you are saying _"the dynamic light striking this surface is not really pointing directly at the surface."_ This is the job of the Quake 3 graphics engine.

You get the most mileage from a specularity map when there are many 
tiny variations in shininess, not one big one like this. Quake 3 will 
probably ship with specularity maps that have many tiny features.

<center><table><tbody><tr><td align="center">
[![](Multitexture/shader.gif)](http://www.bigpanda.com/trinity/shader.psd)
click above to download shader.psd, a shiny wooden wall with a large, shiny raised button and dull grout.
</td></tr></tbody></table></center>

With so many people equating 3DFX with reality these days, it may be 
difficult to visualize Quake 3 as significantly better in ways that are 
important to most game players. Even if i could provide screen shots of 
Quake 3, which i clearly can't, the increase in quality would not be 
obvious. The extra quality conferred by multitexture appears _only_
 when the game is in motion. Once you see this technology in motion for 
the first time, your jaw will drop. Other game engines will seem like an
 enormous waste of time.
</font>

</body></html>


