# Tourbot
### The Friendly Worlds Tour Guide
### Progress: Not finished (teleport needs work)
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

# FAQ
## Can I copy your code?
Yes, but you have to keep the GNU GPL 3 license.
## Where do I report bugs?
Create an issue here on this repository, or send me an email at `bonkmaykr@protonmail.com`.
## Can I deploy this myself on Worlds?
Yes, but please don't try to clog up the servers or chatrooms with a bunch of redundant bots. If Tourbot is working fine, then be nice and let it do it's job unless you can do it better.  
I can't stop you from doing whatever you want with it, but if you do bad things your accounts and IP addresses will probably get banned from Worlds, so use Tourbot wisely.
## When I try to log in, the console says "Account is no longer valid".
Double check your spelling.  
  
If you know your username is spelled correctly and the account hasn't been deleted/banned, make sure you saved bot.conf with Unix line endings. Windows line endings use a really funny looking two-byte control code for new lines, and Tourbot doesn't understand how to read them yet. It probably thinks that your username is something like "John�" instead of "John". Many text editors on Windows allow you to change this setting so Tourbot can read your configs correctly, unfortunately the default notepad.exe doesn't include this.
## Why isn't there a Windows version?
Two reasons.
1. Wirlaburla doesn't use Windows, and when he made P3NG0, his target platform was Linux and nothing else. Refactoring the code to eliminate any cross-platform compatibility issues so it compiles and runs on Windows isn't worth it in my opinion. I host Tourbot on a separate Linux server myself so my daily tasks on my main machine don't interfere with it, I don't have a need to port it.
2. The point of Tourbot is to be an automated chatbot. It's essentially a headless WorldsPlayer client that acts on it's own. It doesn't need a GUI, or anything fancy to work, or to even interact with users. So Tourbot is at home on headless servers, and Linux has always been a lot better for servers because of how flexible it is. Windows is built mostly with end users in mind, and the amount of people hosting VPSes who prefer to use Windows Server is pretty small. There just isn't any benefit to porting the bot. It would take infinitely less effort to just set up WSL on your Windows system and run the bot from there if you absolutely needed a Windows host.
## Why are passwords stored in plaintext? That's not safe!
Worlds doesn't have SSL or TLS, all your traffic to the roomserver is unencrypted. When you start a session on your Worlds account you are transmitting your login details unprotected over the public internet. They aren't hashed on the server either. No token system or anything, this is a very old chat program with very old security standards. Thus there's no point to protecting the passwords when they're sitting on your system.  
  
Ideally you should create a unique password specifically for Tourbot and not share it with anything else. That way, if something bad does happen, your other more secure stuff doesn't get stolen.