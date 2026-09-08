# INPUT BOARD CODE
#### This code is purely for reading the ultrasonic sensor because it requires blocking functions. 

It runs on a secondary Arduino Uno and outputs high to a pin when there is an obstruction. This allows the main board to just read that pin periodically to check for obstruction