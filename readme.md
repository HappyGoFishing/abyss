# The Abyss service manager.
Abyss is a service management daemon for Linux and other Unix systems written in the C programming language.
This project exists because I wanted to improve at C and learn UNIX systems programming. 
## dependencies
[tomlc99](https://github.com/cktan/tomlc99) is [vendored](src/shared/tomlc99) and statically linked.

## Features
- Automatic starting of services on boot
- Service configs declared in TOML files
- very few dependencies


## Features TODO
- dependency management like systemd where services can provide states and require to be launched before or after
- migrate from inhouse dynamic array to stb_ds