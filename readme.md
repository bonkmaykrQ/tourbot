# Tourbot
### The Friendly Worlds Tour Guide
![Work in Progress](https://files.worlio.com/users/bonkmaykr/http/git/embed/pngtree-work-in-progress-png-image_6173846.png)  
Tourbot is an automated chatbot for the Worlds.com online 3D chat service (specifically WorldsPlayer from 1998 and onward). It is based off of P3NG0 by Wirlaburla, the FIRST Worlds chatbot to ever be created and operated live in public. Tourbot retains several features from P3NG0 as well as new features, with the primary selling point being to provide both new and old users an exciting and easy way to explore new worlds, without having to dig through endless archives and file servers like Worlio or jett.dacii.net. This is especially important today as several HOSTs on Worlds have departed or otherwise become inactive, leaving many of the weekly and monthly events abandoned, including tours.  
  
This project is a gift to the Worlds community, and I owe a special thanks to the endless line of nerds who came before me and reverse-engineered the chat protocol so that this could happen.

# Runtime Requirements
- An active internet connection
- A computer running Linux or any other compatible UNIX derivative
- The latest version of glibc
    - The actual version required will depend on the version of glibc linked during compile time.  
    I compile Tourbot on Arch-based distributions which usually have the latest version.  
    If you use a distro such as Ubuntu or Debian then you may need to compile the bot yourself.

# Building Requirements
- CMake
- GCC
- C++ header/source files from Nlohmann's JSON library
    - If you are compiling on an Arch-based Linux system then you can get these instantly by running `sudo pacman -Sy nlohmann-json`
    - Otherwise you just need to put the required `json.hpp` file in your include path and make sure CMake and GCC can see it.

# Building
## Arch Linux / endeavourOS
1. Install nlohmann-json if you haven't already: `sudo pacman -Sy nlohmann-json`
2. Enter the root directory (where `src` is) and run `cmake --build bin/` to create an executable file in the bin folder.
3. `cd` to `bin/` and run `./p3ng0`. (Your working directory needs to be the same directory where `conf/` is located.)

## Other Linux systems
1. Make sure nlohmann JSON is in your INCLUDE path so your compiler can find it's headers.
2. Enter the root directory (where `src` is) and run `cmake --build bin/` to create an executable file in the bin folder.
3. `cd` to `bin/` and run `./p3ng0`. (Your working directory needs to be the same directory where `conf/` is located.)

## Windows
Get WSL or a remote Linux machine and try again.

## Mac
I don't use Mac, I can't help you.