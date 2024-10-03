**Accompanying source code of our EGPGV 2024 paper: Fast Rendering of Parametric Objects on Modern GPUs**     
by Johannes Unterguggenberger, Lukas Lipp, Michael Wimmer, Bernhard Kerbl, and Markus Schütz      
TU Wien

# Setup

Requirements:
- Visual Studio 2022
- MSVC C++ compiler
- Vulkan SDK with VMA header (optional component => select during SDK install)

Setup:
- Clone this repository
- Pull submodules: `git submodule update --init --recursive`
- Open `FastRenderingOfParametricObjects.sln`
- Select the project `FastRenderingOfPaametricObjects` as the startup project
- Build All and wait for the [Post Build Helper](https://github.com/cg-tuwien/Auto-Vk-Toolkit/tree/master/visual_studio#post-build-helper) to have deployed all the assets
- Debug/Run the solution


