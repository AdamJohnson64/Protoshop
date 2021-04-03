# Protoshop

## Inspiration

You want D3D11 then you need some boilerplate code. If you want D3D12 then it's different boilerplate. If it's OpenGL then it's _very_ different boilerplate. I am not a plumber. Boilers are not my thing. Let's write a simple environment to test GPU ideas across the various flavors of APIs, but without duplicating the same boring boilerplate every time.

## Constraints & Features

Here are the design considerations:
- Turning up a new test sample should be FAST.
- Build times should be minimal, and easy to reduce further.
- It should give you the bare minimum you need to try the idea.

We provide a simple syntax to describe "the basic thing that we want" followed by a continuation which is a user-provided function acting on "the thing that is given to us". Currently we have these:

- Give me a D3D11 device with an RTV/DSV and a camera transform.
  - Try some rasterization stuff.
- Give me a D3D11 device with a UAV and a camera transform.
  - Try a compute shader?
- Give me a D3D12 device with an RTV and a camera transform.
  - Try and crash your computer with descriptors.
- Give me a D3D12 device with a UAV and a camera transform.
  - Try and crash your computer with gusto.
  - Or try DXR (which we also do here).
- Give me an OpenGL device.
  - Try some GLSL.
- Give me a Vulkan device.
  - Try some SPIR-V.

We give you:
- Boring math that nobody enjoys writing.
- Boring Win32 window handling that nobody enjoys writing.
- OpenVR if you want it.
- Very loose coupling.

...Then you go play with D3D. Have fun.

Every programmer should have a pet testbed in their back pocket.