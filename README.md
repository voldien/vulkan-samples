# Vulkan Samples #

[![Linux Build](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/voldien/vulkan-samples.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/voldien/vulkan-samples/context:cpp)

A set of vulkan sample for educational purposes.

## Required Packages

```bash
apt install libfmt-dev libglm-dev libsdl2-dev libassimp-dev libfreeimage-dev
```

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make # Build samples
make Shaders # Build Shaders
```

# Window Based Samples

## Startup Window - 
The simplest form of a vulkan window

## Triangle - Vertex Interpolation
Render a single triangle with vertex color interpolation.


## Shader Uniforms - Cube


## Texture - Diffuse Texture
Render a texture cube with simple texture sampling on each face.

## SkyBox - CubeMap
Render Skybox using cubemap, six 2D textures.

## SkyBox - Panoramic
Render skybox using a single texture encoded with equirectangular for projecting to sphere .


## Push Constant - VK_KHR_push_descriptor
Render element using push constant for updating model matrix.

## NormaMap
Render geometry with normal map to add additional details on each face.

## Direction Light
Simple directional light, using older light model.

## Shadow Mapping
Simple shadow map rendering.

## Tessellation Basic
Basic Tessellation on a plane, where geometry is added based on distance as height.

## PN Triangle Tessellation
Tessellation for smoothing out the geometry as subdivsion in runtime. Based on distance.

## Ambient Occlusion
Basic Post Processing for creating ambient occlusion effect.

## Billboard
Create camera facing planes.

## Particle System - Compute Shader
Particle system smoke, compute with compute shader.


## ReactionDiffusion - Compute Shader


## Mandelbrot - Compute Shader 
A Mandelbrot render using compute shader.

## Video Playback

## Memory Benchmark - Benchmarking the performance of transfering 

memory on the device between various heap.
