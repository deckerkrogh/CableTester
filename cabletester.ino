//Decker Krogh
//Autometrix
//2016-2021
//
//2021 updates: 
//		changed pin names to be 0-based
//		put libraries inside src directory

#include "src/Adafruit-GFX-Library/Adafruit_GFX.h"    // Core graphics library
#include <SPI.h>
#include "src/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "src/Adafruit_TouchScreen/TouchScreen.h"
#include <SD.h>

#include "src/MemoryFree/MemoryFree.h"

//Output shift register pins
#define OUT_INV_OE 22
#define OUT_INV_SRCLR 23
#define OUT_SER 24
#define OUT_SRCLK 25
#define OUT_RCLK 26

//Input shift register pins
#define IN_CLK_INH 34
#define IN_QH 35
#define IN_INV_LD 36
#define IN_CLK 37

//Touchscreen analog pins
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 8   // can be a digital pin

//This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

//Touchscreen controls
#define MINPRESSURE 200 // was 10
#define MAXPRESSURE 1000

// The display uses hardware SPI, plus #9 & #10
#define TFT_DC 9
#define TFT_CS 10

//SD card pin
#define SD_CS 4

//Object initialization
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//how many wires it tests
const int numWires = 56;
int inContents[numWires];
bool onHomeScreen = true;

void setup()
{
  //register out pin modes
  pinMode (OUT_INV_OE, OUTPUT);
  pinMode (OUT_INV_SRCLR, OUTPUT);
  pinMode (OUT_SER, OUTPUT);
  pinMode (OUT_SRCLK, OUTPUT);
  pinMode (OUT_RCLK, OUTPUT);

  //register in pin modes
  pinMode (IN_CLK_INH, OUTPUT);
  pinMode (IN_QH, INPUT);
  pinMode (IN_INV_LD, OUTPUT);
  pinMode (IN_CLK, OUTPUT);

  //serial setup
  Serial.begin (9600);

  //allows data out
  digitalWrite (OUT_INV_OE, LOW);
  //prevents clearing
  digitalWrite (OUT_INV_SRCLR, HIGH);

  //doesn't allow data in
  digitalWrite (IN_CLK_INH, HIGH);
  //starts load high
  digitalWrite (IN_INV_LD, HIGH);


  //lcd setup
  tft.begin(0);  //0: triggers spi_default_freq

  //sd card setup
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
  }
  Serial.println("OK!");

  tft.setRotation (2); //rotation changes throughout program due to touchscreen changing orientation and messing things up

  bmpDraw ("auto.bmp", 0, 0);

  Serial.println("READY");

  allWiresLowOut();

  //tft.fillRect (0, 0, 100, 100, ILI9341_RED);

}


void loop()
{
  // Retrieve a point
  TSPoint p = ts.getPoint();


  // valid minimum and maximum pressure
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE)
  {
    return;
  }

  //Serial.print("X = "); Serial.print(p.x);
  //Serial.print("\tY = "); Serial.print(p.y);
  //Serial.print("\tPressure = "); Serial.println(p.z);

  // Scale from ~0->1000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  
  //Serial.println ();
  //Serial.println (p.x);
  //Serial.println (p.y);

  //clock rate
  delay (250);

  if (onHomeScreen)
  {
    homeButtons(p.x, p.y);
  }
  else
  {
    tft.setRotation (2); //2
    bmpDraw ("auto.bmp", 0, 0);
    onHomeScreen = true;
  }
}



//shifts and outputs once
void shiftOne()
{
  digitalWrite (OUT_SRCLK, HIGH);
  delay (1);
  digitalWrite (OUT_SRCLK, LOW);
  digitalWrite (OUT_RCLK, HIGH);
  delay(1);
  digitalWrite (OUT_RCLK, LOW);
  return;
}


//shifts the contents of the input register into array inContents
void inRead ()
{
  int inContentsReversed[numWires + 1];

  digitalWrite (IN_INV_LD, LOW);
  delay(1);
  digitalWrite (IN_INV_LD, HIGH);

  digitalWrite (IN_CLK_INH, LOW);

  for (int c = 0; c < numWires; c++)
  {
    inContentsReversed[c] = digitalRead (IN_QH);
    digitalWrite (IN_CLK, HIGH);
    delay(1);
    digitalWrite (IN_CLK, LOW);
    delay(1);

    //Serial.print (inContentsReversed[c]); //DEBUGGING

  }
  //Serial.println ();
  digitalWrite (IN_CLK_INH, HIGH);


  //Reverses the array because what the register shifts in is reversed
  int c;
  int d;
  for (c = numWires - 1, d = 0; c >= 0; c--, d++)
  {
    inContents[d] = inContentsReversed[c];
    //Serial.print (inContents[d]); // DEBUGGING
  }

  return;
}

void displayInContents()
{
  Serial.println ();
  tft.println ();
  for (int c = 0; c < numWires; c++)
  {
    Serial.print (inContents[c]);
    tft.print (inContents[c]);
  }
  return;
}

int shortWireOne;
int shortWireTwo;

int numInWiresOn_Counter()
{
  int numWiresOn = 0;
  for (int c = 0; c < numWires; c++)
  {
    if (inContents[c] == 1)
    {
      numWiresOn++;
    }
    else
    {
      shortWireOne = c;
    }
  }
  return numWiresOn;
}


void allWiresHighOut()
{
  digitalWrite (OUT_SER, HIGH);
  for (int c = 0; c < numWires; c++)
  {
    delay (1);
    digitalWrite (OUT_SRCLK, HIGH);
    delay (1);
    digitalWrite (OUT_SRCLK, LOW);
  }
  delay (1);
  digitalWrite (OUT_RCLK, HIGH);
  delay (1);
  digitalWrite (OUT_RCLK, LOW);
  return;
}

void allWiresLowOut()
{
  digitalWrite (OUT_SER, LOW);
  for (int c = 0; c < numWires; c++)
  {
    delay (1);
    digitalWrite (OUT_SRCLK, HIGH);
    delay (1);
    digitalWrite (OUT_SRCLK, LOW);
  }
  delay (1);
  digitalWrite (OUT_RCLK, HIGH);
  delay (1);
  digitalWrite (OUT_RCLK, LOW);
  return;
}



//This function prints what is passed to it and saves it so it can print it later (hopefully).
//The isNum boolean is so that it knows to convert ascii-converted integers into normal int
//The isLn boolean adds adds a new line and nothing else
//The color is for color

void tftDisplay(char output[], char color[], bool isNum, bool isLn)
{

  if (isLn)
  {
    tft.println ();
  }
  else
  {
    if (color[0] == 'r') {tft.setTextColor (ILI9341_RED);}
    else {tft.setTextColor (ILI9341_GREEN);}
    
    if (isNum)
    {
      int numOutput = (int)output[0];
      tft.print (numOutput);

    }
    else
    {tft.print (output);}
  }
  return;
}


int connectionTest()
{
  allWiresLowOut();
  bool connectionPassed = true;
  int numConnections = numWires - 6; //the 6 is hardcoded because of the 6 wires inside the cable tester that are connected together
  bool onlyOnOne = true;  //the purpose of this is to only print error message once

  digitalWrite (OUT_SER, HIGH);
  for (int c = 0; c < numWires; c++)
  {
    
    shiftOne();
    digitalWrite (OUT_SER, LOW);
    inRead();
    //displayInContents(); //DEBUGGING    
    
    if (inContents[c] != 1)
    {
      if (onlyOnOne) {tft.print ("Connection test failed on wires: "); tft.println ();}
      onlyOnOne = false;
      
      Serial.println ();
      Serial.print ("Connection test failed on wire: ");
      Serial.print(c+1);  //adding one so the pins are one-based (no 0 pin)
      
      tft.print (c+1);
      tft.print (":");
      
      connectionPassed = false;
      numConnections--;
    }
    
  }
  allWiresLowOut();

  Serial.println  ();
  tft.println ();

  //return 2 if no connections
  if (numConnections == 0)
  {
    return 2;
  }
  else
  {
    if (connectionPassed)
    {
      Serial.println ("CONNECTION TEST PASSED");
      tft.setTextColor (ILI9341_GREEN);
      tft.println ("CONNECTION TEST PASSED");
      return 0;
    }
    else
    {
      Serial.println ("CONNECTION TEST FAILED");
      tft.setTextColor (ILI9341_RED);
      tft.println ("CONNECTION TEST FAILED");
      return 1;
    }
  }
}

int crossTest()
{
  allWiresLowOut();
  tft.println ();

  bool crossPassed = true;
  bool onlyOnOne = true;
  int crossedWires_d[numWires+1]; // array to store numbers of crossed wires that are contained in variable d so it doesn't repeat cross detection
  

  digitalWrite (OUT_SER, HIGH);
  for (int c = 0; c < numWires; c++)
  {
    shiftOne();
    digitalWrite (OUT_SER, LOW);
    inRead();
    if (inContents[c] != 1) // if the one that's outputed high is not high on input then continue down
    {
      for (int d = 0; d < numWires; d++) // loop to see if another one has turned on instead
      {
        //displayInContents(); //DEBUGGING
        
          if ((inContents[d] == 1) && (c != crossedWires_d[d])) // if another one is turned on and it hasn't already been detected as a cross, do the stuff below
          {
            crossedWires_d[c] = d;
          
            if (onlyOnOne) {tft.print ("Wires are crossed:"); tft.println ();}
            onlyOnOne = false;
          
            Serial.print ("Wire "); Serial.print (c+1); Serial.print (" is crossed to wire "); Serial.print (d+1);
            Serial.println ();
            tft.setTextColor (ILI9341_RED);
            tft.print (c+1); tft.print (":"); tft.print (d+1);
            tft.print ("  ");
            crossPassed = false;
        }

      }
    }
  }
  allWiresLowOut();

  Serial.println ();
  tft.println ();
  
  if (crossPassed)
  {
    Serial.println ("CROSS TEST PASSED");
    tft.setTextColor (ILI9341_GREEN);
    tft.println ("CROSS TEST PASSED");
    return 0;
  }
  else
  {
    Serial.println ("CROSS TEST FAILED");
    tft.setTextColor (ILI9341_RED);
    tft.println ("CROSS TEST FAILED");
    return 1;
  }


}

int shortTest()
{
  //there are two tests: one that outputs all high and one that outputs all low
  tft.println ();

  bool shortHighPassed = true;
  bool shortHighMultPassed = true;
  bool shortLowPassed = true;
  bool shortPassed = true;
  int numInWiresOn;
  bool multipleShorts = false; //this means more than two wires shorted together
  int numWiresShorted = 0; // only use this if multiple shorts equals true (for the time being)
  bool onlyOnOne = true; // used for printing values to screen only once
  int shortedWires_d[numWires + 1];

/* commented out because having all led's on at the same time drew too much 'lectricity
  allWiresHighOut();
  digitalWrite (OUT_SER, LOW);
  for (int c = 0; c < numWires; c++) //this short test outputs high on all wires except the one being tested, 
  {                                  //note: if there is a wire that isn't connnected it says that wires that are shorted to the one not connected
    shiftOne();                      
    digitalWrite (OUT_SER, HIGH);
    inRead();
    displayInContents(); //DEBUGGING
    numInWiresOn = numInWiresOn_Counter(); //assigns numInWiresOn to how many wires that read 1

    if ((numInWiresOn < numWires - 1)) //if another wire didn't turn on then it must be shorted
    {
      
      tft.setTextColor (ILI9341_RED);
      
      if (onlyOnOne) {tft.print ("Wires shorted:"); tft.println ();Serial.println (); Serial.print ("Wires shorted:"); Serial.println ();}
      onlyOnOne = false;
      
      for (int d = 0; d < numWires; d++)
      {
        if ((inContents[d] == 0) ) //&& (c != shortedWires_d[d]) && (d != shortedWires_c[c])// can ignore this
         {
          if (c != shortedWires_d[d]) //this checks to see if is has already been outputted and if it has it breaks the loop
          {
          shortedWires_d[c] = d;
          Serial.print (d+1); Serial.print (":");
          tft.print (d+1); tft.print (":");
          }
          else {break;}
        }

      }
      Serial.print ("  ");
      tft.print ("  ");
      
      shortHighPassed = false;
    }
    else if (numInWiresOn == numWires) 
    {
      numWiresShorted++;
      multipleShorts = true;
    }

  }

  if (multipleShorts)
  {
    shortHighMultPassed = false;
    Serial.println ();
    Serial.print (numWiresShorted); Serial.print (" wire short detected");
    tft.setTextColor (ILI9341_RED);
    tft.println ();
    tft.print (numWiresShorted); tft.print (" wire short detected");
  }
  allWiresLowOut();

*/

  digitalWrite (OUT_SER, HIGH);
  onlyOnOne = true;
  for (int c = 0; c < numWires; c++) //this short test outputs low on all wires except the one being tested, if another wire outputs high aswell it throws an error
  {
    shiftOne();
    digitalWrite (OUT_SER, LOW);
    inRead();
    numInWiresOn = numInWiresOn_Counter();
    //displayInContents(); //DEBUGGING

    if (numInWiresOn > 1)
    {
      Serial.println ();
      Serial.print ("Wires ");
      if (onlyOnOne) {tft.print ("Wires shorted:"); tft.println (); onlyOnOne = false;}

      for (int d = 0; d < numWires; d++)
      {
        if (inContents[d] == 1)
        {
          Serial.print (d+1); Serial.print (":");
          tft.print (d+1); tft.print (":");
        }
      }
      Serial.print (" are shorted together");
      shortLowPassed = false;
    }

  }
  allWiresLowOut();

  //returns values
  if ((shortLowPassed) && (shortHighPassed) && (shortHighMultPassed))
  {
    shortPassed;
  }
  else
  {
    shortPassed = false;
  }

  Serial.println ();
  tft.println ();
  
  if (shortPassed)
  {
    Serial.println ("SHORT TEST PASSED");
    tft.setTextColor (ILI9341_GREEN);
    tft.println ("SHORT TEST PASSED");
    return 0;
  }
  else
  {
    Serial.println ("SHORT TEST FAILED");
    tft.setTextColor (ILI9341_RED);
    tft.println ("SHORT TEST FAILED");
    return 1;
  }
}


void homeButtons(int p_x, int p_y)
{
  //home touchscreen buttons (they're upside down)
  if ((p_x > 0) && (p_x < 320) && (p_y > 240) && (p_y < 480)) // the left side of the screen is the testing button
  {

    onHomeScreen = false;

    tft.setRotation (3);
    tft.setCursor (0, 0);
    tft.setTextColor (ILI9341_RED);
    tft.setTextSize (2);
    tft.fillScreen (ILI9341_BLACK);

    int conTestResult = 0;
    int crossTestResult = 0;
    int shortTestResult = 0;

    conTestResult = connectionTest();

    if (conTestResult == 2)
    {
      Serial.println ("NO CONNECTIONS DETECTED");
      tft.println ("NO CONNECTIONS DETECTED");
    }
    else
    {
      crossTestResult = crossTest();
      shortTestResult = shortTest();
      tft.println ();
      tft.setTextSize (3);
      if ((conTestResult == 0) && (crossTestResult == 0) && (shortTestResult == 0))
      {
        Serial.println ("ALL TEST PASSED");
        tft.setTextColor (ILI9341_GREEN);
        tft.println ("ALL TESTS PASSED");
      }
      else
      {
        Serial.println ("TEST FAILED");
        tft.setTextColor (ILI9341_RED);
        tft.println ("TESTS FAILED");
      }
      Serial.print ("freeMemory()=");
      Serial.println (freeMemory());
      loop();
    }

  }
  else if ((p_x > 160) && (p_x < 320) && (p_y > 0) && (p_y < 240)) //bottom right quadrant is shift low
  {

    digitalWrite (OUT_SRCLK, HIGH);
    delay (1);
    digitalWrite (OUT_SRCLK, LOW);
    digitalWrite (OUT_RCLK, HIGH);
    delay (1);
    digitalWrite (OUT_RCLK, LOW);

  }
  else if ((p_x > 0) && (p_x < 160) && (p_y > 0) && (p_y < 240))//top right quadrant is shift high
  {
    digitalWrite (OUT_SER, HIGH);
    delay (1);
    digitalWrite (OUT_SRCLK, HIGH);
    delay (1);
    digitalWrite (OUT_SRCLK, LOW);
    digitalWrite (OUT_RCLK, HIGH);
    delay(1);
    digitalWrite (OUT_RCLK, LOW);
    digitalWrite (OUT_SER, LOW);
  }
}



//Source for below: github.com/Bodmer/TFT_HX8357/blob/master/TFT_HX8357.cpp

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 50

void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if ((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r, g, b));
          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
