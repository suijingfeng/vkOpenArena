# id Tech 3

## Graphics

Unlike most other game engines released at the time, including its primary competitor,
the Unreal Engine, id Tech 3 requires an OpenGL-compliant graphics accelerator to run.
The engine does not include a software renderer.

id Tech 3 introduced spline-based curved surfaces in addition to planar volumes,
which are responsible for many of the surfaces present within the game.


## Shaders

The graphical technology of the game is based tightly around a "shader" system 
where the appearance of many surfaces can be defined in text files referred to
as "shader scripts." Shaders are described and rendered as several layers, each
layer contains a texture, a "blend mode" which determines how to superimpose it
over the previous layer and texture orientation modes such as environment mapping,
scrolling, and rotation. These features can readily be seen within the game with
many bright and active surfaces in each map and even on character models. 

The shader system goes beyond visual appearance, defining the contents of volumes
(e.g. a water volume is defined by applying a water shader to its surfaces),
light emission and which sound to play when a volume is trodden upon. 
In order to assist calculation of these shaders, id Tech 3 implements a specific
fast inverse square root function, which attracted a significant amount of 
attention in the game development community for its clever use of integer operations. 


## Video

In-game videos all use a proprietary format called "RoQ", which was originally
created by Graeme Devine, the co-designer of Quake 3, for the game The 11th Hour.
Internally RoQ uses vector quantization to encode video and DPCM to encode audio.
While the format itself is proprietary it was successfully reverse-engineered in 2001,
and the actual RoQ decoder is present in the Quake 3 source code release.
RoQ has seen little use outside games based on the id Tech 3 or id Tech 4 engines,
but is supported by several video players (such as MPlayer) and a handful of third-party
encoders exist. One notable exception is the Unreal Engine-based game Postal 2:
Apocalypse Weekend, which uses RoQ files for its intro and outro cutscenes,
as well as for a joke cutscene that plays after a mission at the end of part one.


## Models

id Tech 3 loads 3D models in the MD3 format. The format uses vertex movements
(sometimes called per-vertex animation) as opposed to skeletal animation in order to
store animation. The animation features in the MD3 format are superior to those in
id Tech 2's MD2 format because an animator is able to have a variable number of key
frames per second instead of MD2's standard 10 key frames per second. This allows
for more complex animations that are less "shaky" than the models found in Quake II.

Another important feature about the MD3 format is that models are broken up into
three different parts which are anchored to each other. Typically, this is used to
separate the head, torso and legs so that each part can animate independently for
the sake of animation blending (i.e. a running animation on the legs, and shooting
animation on the torso). Each part of the model has its own set of textures.

The character models are lit and shaded using Gouraud shading while the levels
(stored in the BSP format) are lit either with lightmaps or Gouraud shading
depending on the user's preference. The engine is able to take colored lights
from the lightgrid and apply them to the models, resulting in a lighting quality
that was, for its time, very advanced.

In the GPLed version of the source code, most of the code dealing with the MD4
skeletal animation files was missing. It is presumed that id simply never finished
the format, although almost all licensees derived their own skeletal animation
systems from what was present. Ritual Entertainment did this for use in the game,
Heavy Metal: F.A.K.K.², the SDK to which formed the basis of MD4 support completed
by someone who used the pseudonym Gongo.

## Dynamic shadows

The engine is capable of three different kinds of shadows. One just places a circle
with faded edges at the characters' feet, commonly known as the "blob shadow" technique.
The other two modes project an accurate polygonal shadow across the floor.
The difference between the latter two modes is one's reliance on opaque,
solid black shadows while the other mode attempts (with mixed success) to
project depth-pass stencil shadow volume shadows in a medium-transparent black.
Neither of these techniques clip the shadow volumes, causing the shadows to extend
down walls and through geometry.

## Other rendering features

Other visual features include volumetric fog, mirrors, portals, decals, and wave-form vertex distortion. 

## Networking

id Tech 3 uses a "snapshot" system to relay information about game "frames" 
to the client over UDP. The server updates object interaction at a fixed rate
independent of the rate clients update the server with their actions and then
attempts to send the state of all objects at that moment (the current server frame)
to each client. The server attempts to omit as much information as possible about
each frame, relaying only differences from the last frame the client confirmed
as received (Delta encoding). All data packets are compressed by Huffman coding
with static pre-calculated frequency data to reduce bandwidth use even further.

Quake 3 also integrated a relatively elaborate cheat-protection system called "pure server." Any client connecting to a pure server automatically has pure mode enabled, and while pure mode is enabled only files within data packs can be accessed. Clients are disconnected if their data packs fail one of several integrity checks. The cgame.qvm file, with its high potential for cheat-related modification, is subject to additional integrity checks.[citation needed] Developers must manually deactivate pure server to test maps or mods that are not in data packs using the PK3 file format. Later versions supplemented pure server with PunkBuster support, though all the hooks to it are absent from the source code release because PunkBuster is closed source software and including support for it in the source code release would have caused any redistributors/reusers of the code to violate the GPL.

## Virtual machine

id Tech 3 uses a virtual machine to control object behavior on the server, effects and prediction on the client and the user interface. This presents many advantages as mod authors do not need to worry about crashing the entire game with bad code, clients could show more advanced effects and game menus than was possible in Quake II and the user interface for mods was entirely customizable.

Virtual machine files are developed in ANSI C, using LCC to compile them to a 32-bit RISC pseudo-assembly format. A tool called q3asm then converts them to QVM files, which are multi-segmented files consisting of static data and instructions based on a reduced set of the input opcodes. Unless operations which require a specific endianness are used, a QVM file will run the same on any platform supported by Quake 3.

The virtual machine also contained bytecode compilers for the x86 and PowerPC architectures, executing QVM instructions via an interpreter. 
