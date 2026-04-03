# binkw borderless-fullscreen-hack
A mod ("hack") for games that use binkw that allows you to run the game in borderless fulscreen 

For Lego batman, this can be used to fix a bug inwhich cutscenes will freeze for few muniets when they start. This bug seems to be caused by Lego Batman's exlusive fullscreen mode.

This mod works by tricking the game into loading in a fake `binkw32.dll`. This custom .dll creates a new thread that waits for the Lego Batman window to open, then makes the window the size of the current monitor and borderless (borderless fullscreen). It also passes through the original functions.

# How to setup and install
- Force the window to launch in windowed mode, this may very game to game.

For Lego Batman, (properties > general > Launch Options, input "-windowed")

![windowed](windowed.png)

Other games may have a 'windowed' mode in display settings. 

- Open the game directory (properties > installed files > browse)
- Rename the original `binkw32.dll` to `binkw32_real.dll`
- Copy this `binkw32.dll` to the game's directory

![folder](folder.png)

# building
You do not have to build this mod, a pre-compiled binary should be included in the latest release.

That being said, to build you need [MSVC (x86)](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) installed. 

Once you install MSVC (x86), run the Native Tools Command Line and run these instructions: 

![tools image](tools.png)

```sh
cd <this directory>
build.bat 
```
