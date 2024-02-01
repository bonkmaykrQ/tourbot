# Tourbot
### The Friendly Worlds Tour Guide
### Progress: Not finished (teleport needs work)
![Work in Progress](https://files.worlio.com/users/bonkmaykr/http/git/embed/pngtree-work-in-progress-png-image_6173846.png)  
Tourbot is an automated chatbot for the Worlds.com online 3D chat service (specifically WorldsPlayer from 1998 and onward). It is based off of P3NG0 by Wirlaburla, the FIRST Worlds chatbot to ever be created and operated live in public. Tourbot retains several features from P3NG0 as well as new features, with the primary selling point being to provide both new and old users an exciting and easy way to explore new worlds, without having to dig through endless archives and file servers like Worlio or jett.dacii.net. This is especially important today as several HOSTs on Worlds have departed or otherwise become inactive, leaving many of the weekly and monthly events abandoned, including tours.  
  
This project is a gift to the Worlds community, and I owe a special thanks to the endless line of nerds who came before me and reverse-engineered the chat protocol so that this could happen.

# To-Do
Underlined tasks are tasks currently being worked on
- [ ] <ins>Core Functions</ins>
    - [x] Improved configuration system
    - [ ] Strip out old `worldlist.conf` entirely (pair `where` command with `lookUpWorldName()`)
    - [ ] <ins>New commands</ins>
    - [ ] <ins>Teleport-on-request routine</ins>
    - [ ] Basic Tour Guide routine
    - [ ] Tour communication (prioritize leader commands, whispering users out of range, etc)
    - [ ] Ability to join another user's ongoing tour
- [ ] Database Building
- [ ] Spanish Localization
- [ ] Tour Guide Avatar
    - [ ] Contextual Avatars: on tour
    - [ ] Contextual Avatars: idle (advertising)
- [ ] Help Pages, Website, and Documentation
- [ ] Tourbot Status Page
- [ ] New User Recognition and Welcome

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
2. Enter the root directory (where `src` is) and run `cmake -S src/ -B bin/` to create the neccessary files to compile the program.
3. Run `cmake --build bin/` to create an executable file in the bin folder.
4. `cd` to `bin/` and run `./p3ng0`. (Your working directory needs to be the same directory where `conf/` is located.)

## Other Linux systems
1. Make sure nlohmann JSON is in your INCLUDE path so your compiler can find it's headers.
2. Enter the root directory (where `src` is) and run `cmake -S src/ -B bin/` to create the neccessary files to compile the program.
3. Run `cmake --build bin/` to create an executable file in the bin folder.
4. `cd` to `bin/` and run `./p3ng0`. (Your working directory needs to be the same directory where `conf/` is located.)

## Windows
Tourbot isn't designed for Windows right now. As such, I haven't set up the project files to be compilable on Windows, even with Linux as the target. You can either install WSL and compile there, or set up the compiler yourself if you know what you're doing.

## Mac
I don't use Mac and I never plan to support it, I can't help you.

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
  
If you know your username is spelled correctly and the account hasn't been deleted/banned, make sure you saved bot.conf with Unix line endings. Windows line endings use a really funny looking two-byte control code for new lines, and Tourbot doesn't understand how to read them yet. It probably thinks that your username is something like "John�" instead of "John". I plan to fix this if a Windows port is ever released, but for now, you will need to be careful with your .conf files.  
  
Many text editors on Windows allow you to change this setting so Tourbot can read your configs correctly, unfortunately the default notepad.exe doesn't include this.
## Why isn't there a Windows version?
P3NG0 was designed from the very beginning with Linux in mind. When it came time to make Tourbot, the same was true; it's designed to be run headless, 24/7, on a server. Meaning you are expected to have a spare computer handy or a cloud VPS rented in order to run the bot. Just about every sane sysadmin uses Linux for headless systems, since Windows was always optimized around it's user interface and not much else.  
  
I could make a Windows version, doing so is actually not very difficult. The code for Tourbot is pretty small and it very rarely interacts with system APIs if at all. It's just a matter of if I want to or not.
## Why are passwords stored in plaintext? That's not safe!
Worlds doesn't have SSL or TLS, doesn't hash passwords, and doesn't have session tokens or revocable sessions. It's a very old program with extremely outdated security. Tourbot isn't secure because the weakest link in the chain makes any security effort on my part pointless. <ins>If you want to remain safe, use a *unique password* for the bot so that it can't be reused in the event it gets stolen.</ins>