# Ultima IV Recreated

This is a cleaned up, modernized fork of "xu4".

#### How to Compile
Currently, the following dependencies are required:
libxml2, libpng, zlib, libsdl1.2, libsdlmixer1.2
```
make
```

#### Project Goals
* Remove unneeded dependencies
* Greatly simplify the codebase
* Update to use modern libraries
* Convert from ugly C++ to beautiful C
* Render to a texture instead of use SDL directly
* Interally generate all audio instead of using SDL
* Make it "Just Work"
