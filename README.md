# Vulkan Samples #

[![Linux Build](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A collection of Vulkan Samples, work in progress, for personal educational purposes.

## Required Packages

```bash
apt install libfmt-dev libglm-dev libsdl2-dev libassimp-dev libfreeimage-dev
```

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
make DownloadAsset
```

## CLI - Command Line Options

```bash
Usage:
  Vulkan Sample: VulkanSample [OPTION...]

  -h, --help                    helper information.
  -d, --debug                   Enable Debug. (default: true)
  -t, --time arg                How long to run sample (default: 0)
  -i, --instance-extensions arg
                                . (default: 5)
  -l, --instance-layers arg     . (default: 5)
  -E, --device-extensions       .
  -g, --gpu-device arg          GPU Device Select (default: -1)
  -p, --present-mode arg        Present Mode () (default: -1)
  -f, --fullscreen              FullScreen
  -a, --headless                Headless Renderer
  -r, --renderdoc               Enable RenderDoc ()
  -F, --filesystem arg          Set FileSystem, either directory or archive 
                                file (zip) (default: .)
  -C, --color-space arg         Set the Display ColorSpace (Linear,SRGB) 
                                (default: "")
  -W, --width arg               Set Window Width in Pixels (default: -1)
  -H, --height arg              Set Window Height in Pixels (default: -1)
  -D, --display arg             Set Display index where the window will 
                                show (default: -1)
  -m, --multi-sample arg        Set MSAA (Multisampling Anti Aliasing) 
                                (2,4,8) (default: 0)
  -R, --dynamic-range arg       Set Dynamic Range ldr,hdr16,hdr32 (default: 
                                hdr16)
  -P, --use-postprocessing      Use Post Processing (default: true)
```

## Window Based Samples

### Startup Window -

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

## Particle System - Compute Shader

Particle system smoke, compute with compute shader.

## ReactionDiffusion - Compute Shader

## Mandelbrot - Compute Shader

A Mandelbrot render using compute shader.

## Game of Life - Compute Shader

A Mandelbrot render using compute shader.

## Video Playback

## Memory Benchmark - Benchmarking the performance of transfering

memory on the device between various heap.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

Models downloaded from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)

## Misc Notes

### Export Build to Target Directory

```baseh
cmake .. -DCMAKE_INSTALL_PREFIX=/target -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build . --parallel $(nproc --all) --target install ;
```
