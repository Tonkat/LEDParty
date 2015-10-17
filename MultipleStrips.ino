#include <FastLED.h>

#define DATA_PIN    2
#define DATA_PIN_2    4
#define DATA_PIN_3    6
#define DATA_PIN_4    8
#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define NUM_LEDS    240
#define NUM_STRIPS  4

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60

//Sparkling
#define STARTING_BRIGHTNESS 32
#define FADE_IN_SPEED       32
#define FADE_OUT_SPEED      20
#define DENSITY            255

CRGB leds[NUM_LEDS];
CRGB multiLeds[NUM_STRIPS][NUM_LEDS];
CRGBPalette16 gPal;

void setup() {
  delay(3000); // sanity delay
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, DATA_PIN_2, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, DATA_PIN_3, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, DATA_PIN_4, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(multiLeds[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<CHIPSET, DATA_PIN_2, COLOR_ORDER>(multiLeds[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<CHIPSET, DATA_PIN_3, COLOR_ORDER>(multiLeds[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<CHIPSET, DATA_PIN_4, COLOR_ORDER>(multiLeds[3], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( BRIGHTNESS );
  set_max_power_in_volts_and_milliamps(5, 5000);

  Serial.begin(9600);
  
  gPal = HeatColors_p;
  
  MatrixSetup();
  ColorPalSetup();
}

char state = 'a';
void (*looperFunction)() = Fire;

void loop()
{
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random());

  handleStateChange();
  handlePaletteChange();

  looperFunction();
  show_at_max_brightness_for_power();

  delay(10);

  if (Serial.available()) {
    state = Serial.read();
    Serial.println("State changed!");
  }

  Serial.flush();
  /*
  for (int i = 0; i < 100000; i++) {
    Fire();
    show_at_max_brightness_for_power(); // display this frame
    //FastLED.delay(1000 / FRAMES_PER_SECOND);
  }
  */
}

void handleStateChange() {
  if (state < 'a') {
    return;
  }
  if (state == 'a') {
    looperFunction = Fire;
  } else if (state == 'b') {
    looperFunction = FillGradLoop;
  } else if (state == 'c') {
    looperFunction = MatrixLoop;
  } else if (state == 'd') {
    looperFunction = ColorTwinkles;
  } else {
    looperFunction = ColorPalLoop;
  }
}

void handlePaletteChange() {
  if (state < '1') {
    return;
  }
  if (state == '1') {
    gPal = PartyColors_p;
  } else if (state == '2') {
    gPal = RainbowStripeColors_p;
  } else if (state == '3') {
    gPal = CloudColors_p;
  } else if (state == '4') {
    gPal = OceanColors_p;
  } else if (state == '5') {
    gPal = ForestColors_p;
  } else if (state == '6') {
    gPal = HeatColors_p;
  } else {
    gPal = LavaColors_p;
  }
}


// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 100


/*
void rainbowCycle(uint8_t wait) {
  int colorPaletteRange = 256;
  
  for (int j=; j < colorPaletteRange * 5; j++) {
    for (int i=0; i < NUM_LEDS; i++) {
      int color = (j + i) % colorPaletteRange;
      leds[i] = ColorFromPalette(gPal, color);
    }
    FastLED.show();   // write all the pixels out
    FastLED.delay(wait);
  }
}*/

void Fire()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      coolDown(heat, i);
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heatUp(heat, k);
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      igniteSparks(heat, y);
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      mapHeat(heat, j);
    }
}

void coolDown(byte * heat, int index) {
  heat[index] = qsub8( heat[index],  random8(0, ((COOLING * 10) / NUM_LEDS + 2)) );
}

void heatUp(byte * heat, int index) {
  heat[index] = (heat[index - 1] + heat[index - 2] + heat[index - 2] ) / 3;
}

void igniteSparks(byte * heat, int index) {
  heat[index] = qadd8( heat[index], random8(160,255) );
}

void mapHeat(byte * heat, int index) {
  byte colorIndex = scale8( heat[index], 240);
  leds[index] = ColorFromPalette(gPal, colorIndex);
}
//==================================================================
//SpeedTest 
//==================================================================

void SpeedTestLoop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
    show_at_max_brightness_for_power();
  }
  
  /*
  for (int strip = 0; strip < NUM_STRIPS; strip++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      multiLeds[strip][i] = CRGB::Red;
      show_at_max_brightness_for_power();
    }
  }*/
}

//==================================================================
//FillGrad 
//==================================================================

void FillGradLoop() {
  uint8_t starthue = beatsin8(1, 0, 255);
  uint8_t endhue = beatsin8(11, 0, 255);
  if (starthue < endhue) {
    fill_gradient(leds, NUM_LEDS, CHSV(starthue,255,255), CHSV(endhue,255,255), FORWARD_HUES);    // If we don't have this, the colour fill will flip around
  } else {
    fill_gradient(leds, NUM_LEDS, CHSV(starthue,255,255), CHSV(endhue,255,255), BACKWARD_HUES);
  }
  //FastLED.show();
  show_at_max_brightness_for_power();                         // Power managed display of LED's.
}

//==================================================================
//ColorPallete 
//==================================================================

CRGBPalette16 cpCurrentPalette;
CRGBPalette16 cpTargetPalette;
uint8_t maxChanges = 40000; 
TBlendType    cpCurrentBlending;

void ColorPalSetup() {
  cpCurrentPalette = HeatColors_p;                           // RainbowColors_p; CloudColors_p; PartyColors_p; LavaColors_p; HeatColors_p;
  cpTargetPalette = HeatColors_p;                           // RainbowColors_p; CloudColors_p; PartyColors_p; LavaColors_p; HeatColors_p;
  cpCurrentBlending = LINEARBLEND;   
} //setup()


void ColorPalLoop() {
  uint8_t beatA = beatsin8(5, 0, 255);                        // Starting hue
  FillLEDsFromPaletteColors(beatA);
  show_at_max_brightness_for_power();                         // Power managed display.

  EVERY_N_MILLISECONDS(1) {                                // FastLED based timer to update/display the sequence every 5 seconds.
    nblendPaletteTowardPalette( cpCurrentPalette, cpTargetPalette, maxChanges);
  }

  EVERY_N_MILLISECONDS(5000) {                                // FastLED based timer to update/display the sequence every 5 seconds.
    SetupRandomPalette();
  }
} //loop()


void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  uint8_t beatB = beatsin8(15, 10, 20);                       // Delta hue between LED's
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(cpCurrentPalette, colorIndex, 255, cpCurrentBlending);
    colorIndex += beatB;
  }
} //FillLEDsFromPaletteColors()


void SetupRandomPalette() {
  cpTargetPalette = CRGBPalette16(CHSV(random8(), 255, 32), CHSV(random8(), random8(64)+192, 255), CHSV(random8(), 255, 32), CHSV(random8(), 255, 255)); 
} // SetupRandomPalette()

//==================================================================
//ColorTwinkles
//==================================================================

enum { GETTING_DARKER = 0, GETTING_BRIGHTER = 1 };

void ColorTwinkles()
{
  // Make each pixel brighter or darker, depending on
  // its 'direction' flag.
  brightenOrDarkenEachPixel( FADE_IN_SPEED, FADE_OUT_SPEED);
  
  // Now consider adding a new random twinkle
  if( random8() < DENSITY ) {
    int pos = random16(NUM_LEDS);
    if( !leds[pos]) {
      leds[pos] = ColorFromPalette( gPal, random8(), STARTING_BRIGHTNESS, NOBLEND);
      setPixelDirection(pos, GETTING_BRIGHTER);
    }
  }
}

void brightenOrDarkenEachPixel( fract8 fadeUpAmount, fract8 fadeDownAmount)
{
 for( uint16_t i = 0; i < NUM_LEDS; i++) {
    if( getPixelDirection(i) == GETTING_DARKER) {
      // This pixel is getting darker
      leds[i] = makeDarker( leds[i], fadeDownAmount);
    } else {
      // This pixel is getting brighter
      leds[i] = makeBrighter( leds[i], fadeUpAmount);
      // now check to see if we've maxxed out the brightness
      if( leds[i].r == 255 || leds[i].g == 255 || leds[i].b == 255) {
        // if so, turn around and start getting darker
        setPixelDirection(i, GETTING_DARKER);
      }
    }
  }
}

CRGB makeBrighter( const CRGB& color, fract8 howMuchBrighter) 
{
  CRGB incrementalColor = color;
  incrementalColor.nscale8( howMuchBrighter);
  return color + incrementalColor;
}

CRGB makeDarker( const CRGB& color, fract8 howMuchDarker) 
{
  CRGB newcolor = color;
  newcolor.nscale8( 255 - howMuchDarker);
  return newcolor;
}

uint8_t  directionFlags[ (NUM_LEDS+7) / 8];

bool getPixelDirection( uint16_t i) {
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;
  // using Arduino 'bitRead' function; expanded code below
  return bitRead( directionFlags[index], bitNum);
  // uint8_t  andMask = 1 << bitNum;
  // return (directionFlags[index] & andMask) != 0;
}

void setPixelDirection( uint16_t i, bool dir) {
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;
  // using Arduino 'bitWrite' function; expanded code below
  bitWrite( directionFlags[index], bitNum, dir);
  //  uint8_t  orMask = 1 << bitNum;
  //  uint8_t andMask = 255 - orMask;
  //  uint8_t value = directionFlags[index] & andMask;
  //  if( dir ) {
  //    value += orMask;
  //  }
  //  directionFlags[index] = value;
}

//==================================================================
//Lighting
//==================================================================
uint8_t frequency = 0;                                       // controls the interval between strikes
uint8_t flashes = 8;                                          //the upper limit of flashes per strike
unsigned int dimmer = 1;

uint8_t ledstart;                                             // Starting location of a flash
uint8_t ledlen;                                               // Length of a flash

void Lightning() {
  ledstart = random16(NUM_LEDS);           // Determine starting location of flash
  ledlen = random16(NUM_LEDS-ledstart);    // Determine length of flash (not to go beyond NUM_LEDS-1)
  for (int flashCounter = 0; flashCounter < random8(3,flashes); flashCounter++) {
    if(flashCounter == 0) dimmer = 5;     // the brightness of the leader is scaled down by a factor of 5
    else dimmer = random8(1,3);           // return strokes are brighter than the leader
    fill_solid(leds+ledstart,ledlen,CHSV(255, 0, 255/dimmer));
    FastLED.show();                       // Show a section of LED's
    delay(random8(4,10));                 // each flash only lasts 4-10 milliseconds
    fill_solid(leds+ledstart,ledlen,CHSV(255,0,0));   // Clear the section of LED's
    FastLED.show();     
    if (flashCounter == 0) delay (150);   // longer delay until next flash after the leader
    delay(50+random8(100));               // shorter delay between strokes  
  } // for()
  delay(random8(frequency)*100);          // delay between strikes
}


//==================================================================
//Matrix
//==================================================================


// Palette definitions
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending;


// Initialize global variables for sequences
int      thisdelay =  50;                                          // A delay value for the sequence(s)
uint8_t    thishue =  95;
uint8_t    thissat = 255;
int        thisdir =   0;
uint8_t thisbright = 255;
bool        huerot =   0;                                          // Does the hue rotate? 1 = yes
uint8_t      matrixBgclr =   0;
uint8_t      matrixBgbri =   0;

void MatrixSetup() {
  
  currentPalette  = CRGBPalette16(CRGB::Black);
  targetPalette   = RainbowColors_p;                            // Used for smooth transitioning.
  currentBlending = LINEARBLEND;  
}
void MatrixLoop() {
  matrixConfig();
  
  EVERY_N_MILLISECONDS(100) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
  }

  EVERY_N_MILLISECONDS(thisdelay) {
    matrix();
  }
  show_at_max_brightness_for_power();
}

void matrix() {                                               // One line matrix

  if (huerot) thishue++;
  
  if (random16(90) > 80) {
    if (thisdir == 0) leds[0] = ColorFromPalette(currentPalette, thishue, thisbright, currentBlending); else leds[NUM_LEDS-1] = ColorFromPalette( currentPalette, thishue, thisbright, currentBlending);
  }
  else {
    if (thisdir ==0) leds[0] = CHSV(matrixBgclr, thissat, matrixBgbri); else leds[NUM_LEDS-1] = CHSV(matrixBgclr, thissat, matrixBgbri);
  }

  if (thisdir == 0) {
    for (int i = NUM_LEDS-1; i >0 ; i-- ) leds[i] = leds[i-1];
  } else {
    for (int i = 0; i < NUM_LEDS ; i++ ) leds[i] = leds[i+1];
  }
}

void matrixConfig() {                                         // A time (rather than loop) based demo sequencer. This gives us full control over the length of each sequence.
  uint8_t secondHand = (millis() / 1000) % 25;                // Change '25' to a different value to change length of the loop.
  static uint8_t lastSecond = 99;                             // Static variable, means it's only defined once. This is our 'debounce' variable.
  if (lastSecond != secondHand) {                             // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch(secondHand) {
      case  0: thisdelay=50; thishue=95; matrixBgclr=140; matrixBgbri=16; huerot=0; break;
      case  5: targetPalette = OceanColors_p; thisdir=1; matrixBgbri=0; huerot=1; break;
      case 10: targetPalette = LavaColors_p; thisdelay=30; thishue=0; matrixBgclr=50; matrixBgbri=15; huerot=0; break;
      case 15: thisdelay=80; matrixBgbri = 32; matrixBgclr=96; thishue=random8(); break;
      case 20: thishue=random8(); huerot=1; break;
      case 25: break;
    }
  }
}


//==================================================================
//RainbowBeat
//==================================================================

#define qsubd(x, b)  ((x>b)?wavebright:0)                   // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)                          // Analog Unsigned subtraction macro. if result <0, then => 0

// Most of these variables can be mucked around with. Better yet, add some form of variable input or routine to change them on the fly. 1970's here I come. .

// Don't forget to update resetvar() definitions if you change these.
uint8_t wavebright = 128;                                     // You can change the brightness of the waves/bars rolling across the screen. Best to make them not as bright as the sparkles.
uint8_t sineHue = 0;                                          // You can change the starting hue value for the first wave.
uint8_t sineRot = 0;                                          // You can change how quickly the hue rotates for this wave. Currently 0.
uint8_t allsat = 255;                                         // I like 'em fully saturated with colour.
bool sineDir = 0;                                             // You can change direction.
int8_t thisspeed = 10;                                         // You can change the speed, and use negative values.
uint8_t allfreq = 32;                                         // You can change the frequency, thus overall width of bars.
int thisphase = 0;                                            // Phase change value gets calculated.
uint8_t thiscutoff = 192;                                     // You can change the cutoff value to display this wave. Lower value = longer wave.
int sineDelay = 0;                                           // You can change the delay. Also you can change the allspeed variable above. 
uint8_t bgclr = 0;                                            // A rotating background colour.
uint8_t bgbri = 32;                                           // Don't go below 16.
// End of resetvar() redefinitions.

void SineLoop() {
  SineConfig();                                                 // Muck those variables around.
  EVERY_N_MILLISECONDS(sineDelay) {                           // FastLED based non-blocking delay to update/display the sequence.
    one_sin();
    bgclr++;
  }
  show_at_max_brightness_for_power();
} // loop()



void SineConfig() {
  uint8_t secondHand = (millis() / 100000) % 60;                // Increase this if you want a longer demo.
  static uint8_t lastSecond = 99;                             // Static variable, means it's only defined once. This is our 'debounce' variable.
  
  // You can change variables, but remember to set them back in the next demo, or they will stay as is.
  if(lastSecond != secondHand) {
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: sineRot = 1; thiscutoff=252; allfreq=8; break; // Both rotating hues
      case  5: sineRot = 0; thisspeed=-4; break;              // Just 1 rotating hue
      case 10: sineHue = 255; break;                          // No rotating hues, all red.
      case 15: sineRot = 1; thisspeed=2; break;               // 
      case 20: allfreq = 16; break;                           // Time to make a wider bar.
      case 25: sineHue=100; thiscutoff = 96; break;           // Change width of bars.
      case 30: thiscutoff = 96; sineRot = 1; break;           // Make those bars overlap, and rotate a hue
      case 35: thisspeed = 3; break;                          // Change the direction.
      case 40: thiscutoff = 128; wavebright = 64; break;      // Yet more changes
      case 45: wavebright = 128; thisspeed = 3; break;        // Now, we change speeds.
      case 50: thisspeed = -2; break;                         // Opposite directions
      case 55: break;                                         // Getting complicated, let's reset the variables.
    }
  }
} // ChangeMe()



void one_sin() {                                                                // This is the heart of this program. Sure is short.
//  if (thisdir == 0) thisphase+=thisspeed; else thisphase-=thisspeed;          // You can change direction and speed individually.
    thisphase += thisspeed;
    sineHue += sineRot;                                                         // Hue rotation is fun for thiswave.
  for (int k=0; k<NUM_LEDS-1; k++) {
    int thisbright = qsubd(cubicwave8((k*allfreq)+thisphase), thiscutoff);      // qsub sets a minimum value called thiscutoff. If < thiscutoff, then bright = 0. Otherwise, bright = 128 (as defined in qsub)..
    leds[k] = CHSV(bgclr, 255, bgbri);
    leds[k] += CHSV(sineHue, allsat, thisbright);                               // Assigning hues and brightness to the led array.
  }
} // one_sin()


void resetvar() {                                             // Reset the variable back to the beginning.
  wavebright = 128;                                           // You can change the brightness of the waves/bars rolling across the screen. Best to make them not as bright as the sparkles.
  sineHue = 0;                                                // You can change the starting hue value for the first wave.
  sineRot = 0;                                                // You can change how quickly the hue rotates for this wave. Currently 0.
  allsat = 255;                                               // I like 'em fully saturated with colour.
  sineDir = 0;                                                // You can change direction.
  thisspeed = 4;                                              // You can change the speed, and use negative values.
  allfreq = 32;                                               // You can change the frequency, thus overall width of bars.
  thisphase = 0;                                              // Phase change value gets calculated.
  thiscutoff = 192;                                           // You can change the cutoff value to display this wave. Lower value = longer wave.
  thisdelay = 25;                                             // You can change the delay. Also you can change the allspeed variable above. 
  bgbri = 16;
} // resetvar()
