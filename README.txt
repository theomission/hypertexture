README
======

Controls
--------

Normal view:
------------

Space					Open menu 
Left Mouse Button 		Hold and drag to turn
Right Mouse Button 		Hold and drag to roll
Arrow up/down			Change time
Page up/down			Move up/down by 10 units
WASD					Move around
	Shift				Move faster
	Shift+Ctrl 			Move even faster

In menu:
--------

Enter					Activate item
Arrow keys				Move around in the menu
Backspace, left Arrow	Leave current item

when adjusting:
	up/down arrow keys		normal adjustment
	Shift					large adjustment
	Ctrl					small adjustment
	
Description
-----------

This renders density clouds created by running a GLSL shader. The source is 
in the subset of C++11 supported by gcc 4.6.1. 

Volumes can be defined in volumes.txt. There are 3 'noise types' available
at the moment, spherenoise, planenoise and flamenoise. The corresponding
density functions can be found in shaders/gen/*. Some properties can be
changed live using the menu, but some are hardcoded in the shaders.

The "GpuHypertexture" class does most of the rendering work, and can
be found in hyper.cpp. The 3D texture is first generated with a density
function. Then lighting transmittance is precomputed with respect to the
main directional light.  A projection shadow map is computed as well. 
During the main render, the volume is ray marched in the pixel shader 
with a ridiculous number of samples per pixel. 

There are several unused bits of code due to the source being based on a 
common codebase I've been using for small projects. Also, it started life
as a C program and at some point I decided I want to play with C++11, so
there may be some weirdness due to that.

The implementation here is meant for experimenting with density
shapes. For use in a game, the rendering would have to be limited to small
3D textures and be updated infrequently. I'm guessing some other tricks
could be used to skip a lot of the expensive work, but I haven't really
investigated that.



References
==========

The density functions and rendering technique are largely inspired
by the SIGGRAPH 2011 volume rendering course. The relevant notes can be
found at:

http://magnuswrenninge.com/content/pubs/ProductionVolumeRenderingFundamentals2011.pdf

The phase function I used for mie scattering can be found in:

http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html




Attributions
============

I used some bits of other people's software. The copyright for each is 
included below.



webgl-noise
-----------
https://github.com/ashima/webgl-noise

I used most of the classic noise function in shaders/noise.glsl. See
LICENSE-webgl-noise.txt.


Bitstream Vera Font
-------------------
http://www-old.gnome.org/fonts/

Vera.ttf is included in the source. See LICENSE-vera.txt.


stb_truetype
------------
http://nothings.org/stb/stb_truetype.h

Sean Barrett made this public domain, and it's really handy. It's used
here mostly as-is with some minor edits to get rid of warnings.

