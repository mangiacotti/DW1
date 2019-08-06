//Scott Mangiacotti
//Marlborough, Massachusetts USA
//August 2019
//DW1
//8x8 LED Matrix Random Number Display

#include <Time.h>
#include <TimeLib.h>
#include <Colorduino.h>

//Global constants
int const ARRAY_SIZE = 64;

//Global variables
bool gSystemEnabled = false;
bool gSystemPaused = false;
bool gVerboseMessages = false;
int gDutyCycle = 25;
unsigned long gTimeSnapshot = 0;

float gTargetPercent = 0.90;
long gRnd;
int gDistro[ARRAY_SIZE];
bool gToggle = false;

void setup()
{
  //Initialize LED array
  Colorduino.Init();
  initializeDistroArray();
 
  //Open a serial port
  Serial.begin(9600);

  //Setup
  postAppData();

  //Startup enabled
  ableSystem(true);
  
}


void loop()
{
  //Serial port processing
  if (Serial.available() > 0)
  {
    int iControlCode;
    iControlCode = Serial.parseInt();
    processMessage(iControlCode);
  }

  //Determine what to do based on enable status
  if (gSystemEnabled == true && gSystemPaused == false)
  {
    randomize();
    
  }
  else if (gSystemEnabled == true && gSystemPaused == true)
  {
    processPauseState();
    
  }
  
  //Give back processing time
  delay(gDutyCycle);

}


void randomize()
{

  //Initialize randomization
  long lSeed;
  lSeed = (long)second() * (long)minute() * (long)hour() * (long)weekday() * (long)analogRead(0);

  //Comment out one of the two following lines to experiment with different entropy for pseudo-randomness
  //randomSeed(lSeed);
  randomSeed(micros());

  int iDiv;
  int iModulo;

  //Randomly select an LED to energize every other scan
  gToggle = !gToggle;

  if (gToggle == true)
  {
    gRnd = random(0, 64);  //designed for 8x8 RGB Matrix
    incrementDistroArray(gRnd);
    
    iDiv = gRnd / 8;
    iModulo = gRnd % 8;
    Colorduino.SetPixel(iDiv, iModulo, 0, 0, 255);
  }
  else
  {
    Colorduino.ColorFill(0, 0, 0);
  }

  //Paint
  Colorduino.FlipPage();

  //Results
  float fRes;
  fRes = checkDistroArrayResults();

  //Check if we reached the target coverage
  if (fRes >= gTargetPercent)
  {
    coverageResultsAchieved(fRes);
    gTimeSnapshot = millis();
  }
}


void initializeDistroArray()
{
  //Iterate
  for (int i=0; i<ARRAY_SIZE; i++)
  {
    gDistro[i] = 0;
  }

  //Post results
  Serial.println("Array distribution list initialized");
  
}


void incrementDistroArray(int iIndex)
{
  //Validate
  if (iIndex < 0 || iIndex >= ARRAY_SIZE)
  {
    return;
  }

  //Increment
  gDistro[iIndex]++;

  //Post results
  if (gVerboseMessages == true)
  {
    Serial.print("Distro array index: ");
    Serial.print(iIndex);
    Serial.print(". New value: ");
    Serial.println(gDistro[iIndex]);
  }
  
}


//Return value in decimal. 14% to be returned as 0.14
float checkDistroArrayResults()
{
  int iCount;
  float fResult;

  //Initialize
  iCount = 0;
  
  //Iterate
  for (int i=0; i<ARRAY_SIZE; i++)
  {
    if (gDistro[i] > 0)
    {
      iCount++;
    }
  }

  //Check results
  fResult = ((float)iCount/(float)ARRAY_SIZE);

  if (gVerboseMessages == true)
  {
    Serial.print("Success count: ");
    Serial.print(iCount);
    Serial.print(". Percent: ");
    Serial.println(fResult);
  }

  return fResult;
}


void coverageResultsAchieved(float fResPct)
{
  //ilmo
  int iDiv;
  int iModulo;

  //Clear display in prep for displaying results
  Colorduino.ColorFill(0, 0, 0);
  Colorduino.FlipPage();

  //Iterate
  for (int i=0; i<ARRAY_SIZE; i++)
  {
    //Determine row and column from array index
    iDiv = i / 8;
    iModulo = i % 8;

    //Illuminate LED  
    if (gDistro[i] > 0)
    { //green this value was randomly selected
      Colorduino.SetPixel(iDiv, iModulo, 0, 255, 0);
    }
    else
    { //red this value was not randomly selected
      Colorduino.SetPixel(iDiv, iModulo, 255, 0, 0);
    }
    
  }

  //Paint
  Colorduino.FlipPage();

  //Post results
  Serial.print(fResPct * 100.00);
  Serial.println("% coverage achieved. Pausing system.");

  //Pause
  gSystemPaused = true;
  
}


void processPauseState()
{

  unsigned long lNow;
  unsigned long lDelta;
  
  //Initialize
  lNow = millis();

  //Calculate time difference in milliseconds
  lDelta = lNow - gTimeSnapshot;

  //Compare
  if (lDelta > 14000) //14000 milliseconds or 14 seconds
  {
    initializeDistroArray();
    gSystemPaused = false;
    Serial.println("Pause time expired. Restart Randomizing");
  }
  
}


void ableSystem(bool bEnable)
{
  if (bEnable == true)
  {
    //ilmoReset all counters in preparation for starting
    initializeDistroArray();
    Colorduino.ColorFill(0, 0, 0);

    //Set parameters as needed
    gSystemEnabled  = true;
    gSystemPaused = false;

    //Post results
    Serial.print("System enabled. Target: ");
    Serial.print(gTargetPercent*100.00);
    Serial.println("%");
  }
  else
  {
    //Disable system
    gSystemEnabled  = false;
    gSystemPaused = false;
    Colorduino.ColorFill(0, 0, 0);

    //Post results
    Serial.println("System disabled");
  }

  //Paint
  Colorduino.FlipPage();
  
}


//Read data from serial port and process message from user
//Format is: XXnnnn
//XX is a value between 1 - 32 and represents the command type or area (for example manual commands to the HOUR servo motor)
//nnnn is a value between 0-1000 and represents the value for the target command type
//For example, 01180 is type 02 and value 180. It represents HOUR servo motor move to position 180 degrees
//See documentation for command definitions and value ranges
void processMessage(int iMessage)
{
  int iControlCode;
  int iControlValue;

  //Process the serial port message
  if (iMessage > 0)
  {
    iControlCode = iMessage / 1000;
    iControlValue = iMessage % 1000;
  }

  //Misc control and command codes
  if (iControlCode == 10)
  {
    if (iControlValue == 0)
    {
      postAppData();
      
    }
    //Control codes and commands
    else if (iControlValue == 1)
    {
      if (gSystemEnabled == false)
      {
        ableSystem(true);  
      }
      else
      {
        Serial.println("System already enabled");
      }
      
    }
    else if (iControlValue == 2)
    {
      ableSystem(false);  

    }
    else if (iControlValue == 3)
    {
      gVerboseMessages = !gVerboseMessages;

      if (gVerboseMessages == false)
      {
        Serial.println("Verbose mode disabled");
      }
      else
      {
        Serial.println("Verbose mode enabled");
      }
    }
    else if (iControlValue == 4)
    { //report all settings

      Serial.print("Target percent = ");
      Serial.print(gTargetPercent*100.00);
      Serial.println("%");
      //Iterate array ilmo
      for (int i=0; i<ARRAY_SIZE;  i++)
      {
        Serial.print("LED[");
        Serial.print(i);
        Serial.print("] = ");
        Serial.println(gDistro[i]);
      }
      
           
    }
    else
    {
      Serial.print("Invalid Control Value: ");
      Serial.println(iControlValue);
    }
  }

  if (iControlCode == 11)
  { //New Target Percent
    if (iControlValue >= 10 && iControlValue <= 100)
    {
      gTargetPercent = (float)iControlValue / 100.0;
      Serial.print("Target percent changed to: ");
      Serial.print(gTargetPercent*100);
      Serial.println("%");
    }
    else
    {
      Serial.print("New Target Percent Value out of range: ");
      Serial.println(iControlValue);
    }
  }

  //Draw separator
  Serial.println("-----");

}


void postAppData()
{
  Serial.println("DW1");
  Serial.println("8x8 LED Matrix Random Number Display");
  Serial.println("By Scott Mangiacotti");
  Serial.println("Marlborough, Massachusetts USA");
  Serial.println("August 2019");
}
