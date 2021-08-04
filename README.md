Decker Krogh
Autometrix
2016 - 2021

This is the arduino code for an internal cable testing tool. All of it can be found within the cableTester.ino file.

To edit the code the Arduino IDE can be downloaded from their site. Alternatively, there is an arduino-cli tool offered.

## Libraries:
The cabletester.ino sketch doesn't actually use the libraries contained in the cabletester/libraries folder but instead the libraries in the user's sketchbook folder. cabletester/libaries serves as a backup of libraries that are known to be compatible with the sketch. Thus, it is recommended to copy the contents of cabletester/libraries into the sketchbook libraries folder so as not potentially break compatibility. Kind of a pain, but Arduino does not offer any efficient way of bundling libraries with code.
