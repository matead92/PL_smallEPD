/* *****************************************************************************************
PL_smallEPD - A library for 1.1”, 1.4", 2.1" and 3.1" E-Paper displays (EPDs) from 
Plastic Logic based on UC8156 driver IC for Adafruit GFX core library. The communication is 
SPI-based, for more information about hook-up please check: https://github.com/plasticlogic.

Created by Robert Poser, Mar 30th 2021, Dresden/Germany. Released under BSD license
(3-clause BSD license), check license.md for more information.

We invested time and resources providing this open source code, please support Plasticlogic 
and open source hardware by purchasing this product @Plasticlogic
***************************************************************************************** */
#include "PL_smallEPD.h"

PL_smallEPD::PL_smallEPD(int8_t _cs, int8_t _rst, int8_t _busy) : Adafruit_GFX(EPD_WIDTH, 
EPD_HEIGHT) {

    cs      = _cs;
    rst     = _rst;
    busy    = _busy;
}

// PUBLIC

// ******************************************************************************************
// BEGIN - Resetting UC8156 driver IC and configuring all sorts of behind-the-scenes-settings
// By default (WHITEERASE=TRUE) a clear screen update is triggered once to erase the screen.
// ******************************************************************************************
void PL_smallEPD::begin(int8_t BGcolor) {
    pinMode(cs, OUTPUT);  
    if (busy!=-1)
        pinMode(busy, INPUT);

    if (rst!=-1) {
        pinMode(rst, OUTPUT);                   //Trigger a global hardware reset...
        digitalWrite(rst, HIGH);  
        delay(5);   
        digitalWrite(rst, LOW);
        delay(5);        
        digitalWrite(rst, HIGH);
        delay(5);        
        waitForBusyInactive();
    } else
        writeRegister(EPD_SOFTWARERESET, -1, -1, -1, -1);    //... or do software reset if no pin defined

    _EPDsize=getEPDsize();                                  //Read NVM to determine display size
    switch (_EPDsize) {
/*        case 11:
            _width=72; _height=148; nextline= _width/4; _buffersize=_width*_height/4;
            width=148; height=72;
            writeRegister(EPD_PANELSETTING, 0x12, -1, -1, -1);        
            writeRegister(EPD_WRITEPXRECTSET, 0, 71, 0, 147);
            writeRegister(EPD_VCOMCONFIG, 0x00, 0x00, 0x24, 0x07);
            break;
        case 14:
            _width=100; _height=180; nextline= _width/4; _buffersize=_width*_height/4;
            width=180; height=100;
            writeRegister(EPD_PANELSETTING, 0x12, -1, -1, -1);        
            writeRegister(EPD_WRITEPXRECTSET, 0, 0xB3, 0x3C, 0x9F);
            writeRegister(EPD_VCOMCONFIG, 0x00, 0x00, 0x24, 0x07);
            break;
  */      case 21:
            _width=146; _height=240; nextline= _width/4; _buffersize=_width*_height/4;
            width=240; height=146;
            writeRegister(EPD_PANELSETTING, 0x10, -1, -1, -1);        
            writeRegister(EPD_WRITEPXRECTSET, 0, 239, 0, 145);  //147?
            //writeRegister(EPD_VCOMCONFIG, 0x00, 0x00, 0x24, 0x07);
            writeRegister(EPD_VCOMCONFIG, 0x00, 0x00, 0x24, 0x05);

            break;
  /*      case 31:
            _width=76; _height=312; nextline= _width/4; _buffersize=_width*_height/4;
            width=312; height=76;
            writeRegister(EPD_PANELSETTING, 0x12, -1, -1, -1);        
            writeRegister(EPD_WRITEPXRECTSET, 0, 0x97, 0, 0x9b);  
            writeRegister(EPD_VCOMCONFIG, 0x50, 0x01, 0x24, 0x07);
  */  }
   // writeRegister(0x1B, 0xA7, 0x04, -1, -1);
    writeRegister(EPD_DRIVERVOLTAGE, 0x25, 0xff, -1, -1);
    writeRegister(EPD_BORDERSETTING, 0x04, -1, -1, -1);
    writeRegister(EPD_LOADMONOWF, 0x60, -1, -1, -1);
    writeRegister(EPD_INTTEMPERATURE, 0x0A, -1, -1, -1);
    writeRegister(EPD_BOOSTSETTING, 0x22, 0x17, -1, -1);

    setRotation(1);                             //Set landscape mode as default
    clearScreen(BGcolor);                   //Start with a white refresh if TRUE
    setTextColor(EPD_BLACK);                    //Set text color to black as default
}

// ************************************************************************************
// CLEAR - Erases the image buffer and triggers an image update and sets the cursor
// back to the origin coordinates (0,0).
// ************************************************************************************
void PL_smallEPD::clear(byte c, bool b2) {
  switch (c) {
    case EPD_WHITE:
      for (int i=0; i<_buffersize; i++) {
          if (!b2) buffer[i] = 0xff;
          buffer2[i] = 0xff;
      }
      break;
    case EPD_LGRAY:
      for (int i=0; i<_buffersize; i++) {
          if (!b2) buffer[i] = 0xaa;
          buffer2[i] = 0xaa;
      }
      break;
    case EPD_DGRAY:
      for (int i=0; i<_buffersize; i++) {
          if (!b2) buffer[i] = 0x55;
          buffer2[i] = 0x55;
      }
      break;
    case EPD_BLACK:
      for (int i=0; i<_buffersize; i++) {
          if (!b2) buffer[i] = 0x00;
          buffer2[i] = 0x00;
      }
      break;
    }
    setCursor(0,0);
}

// ************************************************************************************
// WHITE ERASE - Triggers two white updates to erase the screen and set back previous
// ghosting. Recommended after each power cycling.
// ************************************************************************************
void PL_smallEPD::clearScreen(int8_t BGcolor) {
    clear();
    if (BGcolor>=0) {
        if (BGcolor == EPD_BLACK)
            invert();
        update();
        invert();
        update();
        invert();
        update();       
    }
}

// ************************************************************************************
// DRAWPIXEL - Draws pixel in the memory buffer at position X, Y with the value of the
// parameter color (2 bit value).
// ************************************************************************************
void PL_smallEPD::drawPixel(int16_t x, int16_t y, uint16_t color) {

    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height) || (color>4 )) return;  

    if (_EPDsize==11 || _EPDsize==3) 
        y=y+3;
    uint8_t pixels = buffer[x/4 + (y) * nextline];
  switch (x%4) {                      //2-bit grayscale dot
      case 0: buffer[x/4 + (y) * nextline] = (pixels & 0x3F) | ((uint8_t)color << 6); break;  
      case 1: buffer[x/4 + (y) * nextline] = (pixels & 0xCF) | ((uint8_t)color << 4); break;  
      case 2: buffer[x/4 + (y) * nextline] = (pixels & 0xF3) | ((uint8_t)color << 2); break;  
      case 3: buffer[x/4 + (y) * nextline] = (pixels & 0xFC) | (uint8_t)color; break;   
  }
}

void PL_smallEPD::drawPixel2(int x, int y, int color) {
   if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height) || (color>4 )) return;  
    
    uint8_t pixels = buffer2[x/4 + (y) * nextline];
  switch (x%4) {                      //2-bit grayscale dot
      case 0: buffer2[x/4 + (y) * nextline] = (pixels & 0x3F) | ((uint8_t)color << 6); break; 
      case 1: buffer2[x/4 + (y) * nextline] = (pixels & 0xCF) | ((uint8_t)color << 4); break; 
      case 2: buffer2[x/4 + (y) * nextline] = (pixels & 0xF3) | ((uint8_t)color << 2); break; 
      case 3: buffer2[x/4 + (y) * nextline] = (pixels & 0xFC) | (uint8_t)color; break;    
  }
}

int PL_smallEPD::getPixel(int x, int y) {
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return 5;  

  uint16_t byteIndex = x/4 + (y) * nextline;
    switch (x%4) {        
        case 0: return ((unsigned int)(buffer[byteIndex] & 0xC0) >> 6); 
        case 1: return ((unsigned int)(buffer[byteIndex] & 0x30) >> 4);
        case 2: return ((unsigned int)(buffer[byteIndex] & 0x0C) >> 2);
        case 3: return ((unsigned int)(buffer[byteIndex] & 0x03)); 
  }
  return 0;
}

// ************************************************************************************
// INVERT - Inverts the screen content from black to white and vice versa
// ************************************************************************************
void PL_smallEPD::invert(bool b2) {
    for (int i=0; i<_buffersize; i++) 
        if (b2)
            buffer2[i] = ~buffer2[i];
        else
            buffer[i] = ~buffer[i];
}

// ************************************************************************************
// UPDATE - Triggers an image update based on the content written in the image buffer.
// There are three different updateModes supported: EPD_UPD_FULL(0) is set by default,
// achieves four greyelevels, takes about 800ms and refreshes all pixels. This is the
// update mode having the best image quality. EPD_UPD_PART(1) is a variant of the
// previous one but only changing pixels are refreshed. This results in less flickering
// for the price of a slightly higher pixel to pixel crosstalk. EPD_UPD_MONO(2) is
// again a variant of the previous update mode but only about 250ms long. this allows
// slightly faster and more responsive updates for the price of only two greylevels
// being supported (EPD_BLACK and EPD_WHITE). Depending on your application it is
// recommended to insert a full update EPD_UPD_FULL(0) after a couple of mono updates
// to increase the image quality.
// THIS KIND OF DISPLAY IS NOT SUITED FOR LONG RUNNING ANIMATIONS OR APPLICATIONS WITH
// CONTINUOUSLY HIGH UPDATE RATES. AS A RULE OF THUMB PLEASE TRIGGER UPDATES IN AVERAGE
// NOT FASTER THAN MINUTELY (OR RUN BACK2BACK UPDATES NOT LONGER AS ONE HOUR PER DAY.)
// ************************************************************************************
void PL_smallEPD::update(int updateMode, byte coovl, bool manPow) {
    updateLectum(updateMode, manPow);
}

void PL_smallEPD::updateLectum(int updateMode, bool manPow) {
    scrambleBuffer();
    writeBuffer();
    if (!manPow) 
        powerOn();
    switch (updateMode) {
        case 0:
            writeRegister(EPD_PROGRAMMTP, 0x00, -1, -1, -1);
            writeRegister(EPD_DISPLAYENGINE, 0x03, -1, -1, -1);
            waitForBusyInactive();
            break;
        case 1:
            writeRegister(EPD_PROGRAMMTP, 0x00, -1, -1, -1);
            writeRegister(EPD_DISPLAYENGINE, 0x03, -1, -1, -1);
            waitForBusyInactive();
            break;
        case 2:
            writeRegister(EPD_PROGRAMMTP, 0x02, -1, -1, -1);
            writeRegister(EPD_DISPLAYENGINE, 0x03, -1, -1, -1);
            waitForBusyInactive();
    }
    if (!manPow) 
        powerOff();
}

// ************************************************************************************
// SCRAMBLEBUFFER - 
// ************************************************************************************
void PL_smallEPD::scrambleBuffer() {
    switch (_EPDsize) {
        case 21:
            for (int y=0; y<146; y++) {                   // for each gateline...
                for (int x=0; x<240/2; x++) {             // for each sourceline...
                    drawPixel2(239-x, y, getPixel(x,y+1));
                    drawPixel2(x, y, getPixel(x+120,y+1));
                }
            }
            break;
        case 31:
            for (int y=0; y<_height; y++) {               // for each gateline...
                //scrambleline=
                for (int x=0; x<_width; x++) {            // for each sourceline...
                    if (x%2)                                    // if x = 1, 3, 5..
                        drawPixel2((x-1)/2, y+1, getPixel(x,y));
                    else
                        drawPixel2(_width/2+x/2, y, getPixel(x,y));
                }
            }
    }
}

// ************************************************************************************
// SETROTATION - Let’s you define the display orientation. If set to “1” the landscape
// mode is select (default), if set to “2” the display is set to portrait mode.
// ************************************************************************************
void PL_smallEPD::setRotation(uint8_t o) {
    clear();
    if (o==1) {
        nextline = _height/4;             //Landscape mode (default)
        switch (_EPDsize) {
            case 11:
                writeRegister(EPD_DATENTRYMODE, 0x07, -1, -1, -1); 
                break;
            case 14:
                writeRegister(EPD_DATENTRYMODE, 0x02, -1, -1, -1); 
                break;
            case 21:
                writeRegister(EPD_DATENTRYMODE, 0x20, -1, -1, -1);  
                break;
            case 31:
                writeRegister(EPD_DATENTRYMODE, 0x07, -1, -1, -1);  
        }
        
        _width  = _width + _height;
        _height = _width - _height;
        _width  = _width - _height;
    }
    if (o==2) {
        nextline = _width/4;     //Portrait mode
        writeRegister(EPD_DATENTRYMODE, 0x02, -1, -1, -1);
        _width  = _width;
        _height = _height;
  }
}


// ************************************************************************************
// SETVBORDERCOLOR - Sets the color of the VBorder around the active area. By default
// this is set to White (matching to the Paperino FrontCover) and should not be changed
// ************************************************************************************
void PL_smallEPD::setVBorderColor(int color) {
    if (color==3) writeRegister(EPD_BORDERSETTING, 0xF7, -1, -1, -1);  //
    if (color==0) writeRegister(EPD_BORDERSETTING, 0x07, -1, -1, -1);  //
    update(EPD_UPD_PART);
    writeRegister(EPD_BORDERSETTING, 0x04, -1, -1, -1);  //
}


void PL_smallEPD::writeToPreviousBuffer(){
    writeBuffer(true);
}





// ************************************************************************************
// READTEMPERATURE - Uses the built-in temperature sensor to read back the actual
// temperature; the value is auto-retrieved at the beginning of an image update. Its
// not that good, accuracy seems to be around ±2C, better useful for playing around
// rather than professional temp monitoring.
// ************************************************************************************
uint8_t PL_smallEPD::readTemperature() {
    uint8_t temp;
    digitalWrite(cs, LOW);
    SPI.transfer(EPD_REGREAD | 0x08);
    temp = SPI.transfer(0xFF);
    digitalWrite(cs, HIGH);
    waitForBusyInactive();
    return temp;
}

// PRIVATE

// ************************************************************************************
// POWERON - Activates the defined high voltages needed to update the screen. The
// command should always be called before triggering an image update.
// ************************************************************************************
void PL_smallEPD::powerOn() {
    waitForBusyInactive();
    switch (_EPDsize) {
        case 11:
            writeRegister(EPD_SETRESOLUTION, 0, 239, 0, 147);
            break;
        case 14:
            writeRegister(EPD_SETRESOLUTION, 0, 0xEF, 0, 0x9F);
            break;
        case 21:
            writeRegister(EPD_SETRESOLUTION, 0, 239, 0, 159);
            break;
        case 31:
            writeRegister(EPD_SETRESOLUTION, 0, 239, 0, 159);
    }
    writeRegister(EPD_TCOMTIMING, 0x67, 0x55, -1, -1);
    writeRegister(EPD_POWERSEQUENCE, 0x00, 0x00, 0x00, -1);
    writeRegister(EPD_POWERCONTROL, 0xD1, -1, -1, -1);
    while (readRegister(0x15) == 0) {}          // Wait until Internal Pump is ready    
}

// ************************************************************************************
// POWEROFF - Deactivates the high voltages needed to update the screen. The
// command should always be called after triggering an image update.
// ************************************************************************************
void PL_smallEPD::powerOff() {
    writeRegister(EPD_POWERCONTROL, 0xD0, -1, -1, -1);
    waitForBusyInactive();
    writeRegister(EPD_POWERCONTROL, 0xC0, -1, -1, -1);
    waitForBusyInactive();
}


// ************************************************************************************
// WRITEBUFFER - Sends the content of the memory buffer to the UC8156 driver IC.
// ************************************************************************************
void PL_smallEPD::writeBuffer(bool previous){
 
    writeRegister(EPD_PIXELACESSPOS, 0, 0, -1, -1); 
    if (previous)
        writeRegister(EPD_DATENTRYMODE, 0x30, -1, -1, -1);        //Previous buffer @UC8156
    else
        writeRegister(EPD_DATENTRYMODE, 0x20, -1, -1, -1);        

    
    digitalWrite(cs, LOW);
    SPI.transfer(0x10);
    if (_EPDsize==31 or _EPDsize==21)
        for (int i=0; i < _buffersize; i++) 
            SPI.transfer(buffer2[i]);
    else
        for (int i=0; i < _buffersize; i++) 
            SPI.transfer(buffer[i]);
    digitalWrite(cs, HIGH);
    waitForBusyInactive();
}


// ************************************************************************************
// WRITE REGISTER - Sets register ADDRESS to value VAL1 (optional: VAL2, VAL3, VAL4)
// ************************************************************************************
void PL_smallEPD::writeRegister(uint8_t address, int16_t val1, int16_t val2, 
    int16_t val3, int16_t val4) {
    digitalWrite(cs, LOW);
    SPI.transfer(address);
    if (val1!=-1) SPI.transfer((byte)val1);
    if (val2!=-1) SPI.transfer((byte)val2);
    if (val3!=-1) SPI.transfer((byte)val3);
    if (val4!=-1) SPI.transfer((byte)val4);
    digitalWrite(cs, HIGH);
    waitForBusyInactive();
}

// ************************************************************************************
// READREGISTER - Returning the value of the register at the specified address
// ************************************************************************************
byte PL_smallEPD::readRegister(char address){
    byte data;
    digitalWrite(cs, LOW);                                      
    SPI.transfer(address | EPD_REGREAD);
    data = SPI.transfer(0xFF);                         
    digitalWrite(cs, HIGH);
    waitForBusyInactive();
    return data;                                        // can be improved
}

void PL_smallEPD::loadImg(const unsigned char *pic_name) {
      for (uint16_t j=0; j < sizeof(buffer); j++) {     //144 bytes
          buffer[j] = pgm_read_byte_near(pic_name + j);
      }
}
// ************************************************************************************
// GETEPDSIZE - Returns the size of the attached display diagonal, e.g. 11 is 
// equivalent to to a 1.1" EPD, 21 correpsonds to 2.1" and 31 is equal to 3.1" EPD size
// ************************************************************************************
byte PL_smallEPD::getEPDsize(){
    byte data;
    writeRegister(EPD_PROGRAMMTP, 0x02, -1, -1, -1);    // Set MTP2 as active
    writeRegister(EPD_MTPADDRESSSETTING, 0xF2, 0x04, -1, -1); 
                                                        // Set MTP address to 0x04F0h
    data = readRegister(0x43);                          // Read one dummy byte
    data = readRegister(0x43);                          // Read one byte

    if (data==49) {
        data = readRegister(0x43);                      
        if (data==49)                                   // 1.1" detected
            return 11;
        else 
            return 14;                                  // 1.4" detected
    } 
       
    if (data==50)                                       // 2.1" detected
        return 21;
        
    if (data==51)                                       // 3.1" detected
        return 31;
        
    return 99;
}




// ************************************************************************************
// WAITFORBUSYINACTIVE - Sensing to ‘Busy’ pin to detect the UC8156 driver status.
// Function returns only after driver IC is free again for listening to new commands.
// ************************************************************************************
void PL_smallEPD::waitForBusyInactive(){
    while (digitalRead(busy) == LOW) {}
}

// ************************************************************************************
// DEEPSLEEP - Putting the UC8156 in deep sleep mode with less than 1µA current @3.3V.
// Reset pin toggling needed to wakeup the driver IC again.
// ************************************************************************************
void PL_smallEPD::deepSleep(void) {
    writeRegister(0x21, 0xff, 0xff, 0xff, 0xff); 
}
