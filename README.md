**Accompanying source code of our EGPGV 2024 paper: Fast Rendering of Parametric Objects on Modern GPUs**     
by Johannes Unterguggenberger, Lukas Lipp, Michael Wimmer, Bernhard Kerbl, and Markus Schütz      
TU Wien

![Screenshot of the UI section to enable/disable parametric objects](parametric_objects_screenshot.png "Parametric Objects UI section")      
_Figure 1:_ A screenshot of the UI section which allows enabling and disabling of different kinds of parametric objects.

# Setup

Requirements:
- Visual Studio 2022
- MSVC C++ compiler
- Vulkan 1.3 or 1.4 SDK with VMA header (optional component => select during SDK install)
- A GPU with at least 3 gigabytes of video memory*

*) Some buffers are pre-allocated, totalling approximately 3 gigabytes. See `MAX_VERTICES` and `MAX_INDICES` in code.

Setup:
- Clone this repository
- Pull submodules: `git submodule update --init --recursive`
- Open `FastRenderingOfParametricObjects.sln`
- Select the project `FastRenderingOfPaametricObjects` as the startup project
- Build All and wait for the [Post Build Helper](https://github.com/cg-tuwien/Auto-Vk-Toolkit/tree/master/visual_studio#post-build-helper) to have deployed all the assets
- Debug/Run the solution


