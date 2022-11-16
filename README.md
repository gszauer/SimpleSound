# SimpleSound

![2D desktop demo](images/sample.png)

# API / Design

# Backends

# Spatialization

Spatialization is done in two parts. First, figure out how much each speaker needs to be panned. Second, figure out how much to attenuate the sound.

An easy way to [spatialize sound](https://web.archive.org/web/20220212193327/https://ourmachinery.com/post/writing-a-low-level-sound-system/)  is [Linear Panning](https://docs.unrealengine.com/5.0/en-US/spatialization-overview-in-unreal-engine/). [Vector Base Amplitude Panning](https://www.amazon.com/dp/149874673X) sounds better but is more difficult to implement (and only works with 2 or more speakers). Instead, spatialization is done the way [SoLoud](https://sol.gfxile.net/soloud/) handles audio spatialization.