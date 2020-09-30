#include <Arduino.h>

#include <LiquidCrystal.h>
#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include <Wire.h>

// http://www.qc-lcd.com/Upload/file/20138519401472480.pdf
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 17, en = 16, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const int chipSelect = 10;
// these are the button pins
// const int b1 = 1, b2 = 8, b3 = 20, b4 = 21; // 1, 14, 9, 22
const int b1 = 1, b2 = 9, b3 = 14, b4 = 22;
int b3_val = 0;
int b4_val = 0;

File root;
File gn_dir;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

AudioPlaySdWav           playWav1;
// Use one of these 3 output types: Digital I2S, Digital S/PDIF, or Analog DAC
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  11
#define SDCARD_SCK_PIN   13

int max_samples = 100;

// string array for holding all the file names
String wav_names[100];

// this array holds 1 if sample should be looped
unsigned int is_loop[100];

// holds the lengths in ms
unsigned long wav_lengths[100];

// name of all/currently playing wav
String wavnames[100];
String wavname;
int str_len;

// Assume the name of the file is less than 100 chars
char name_arr[100];
// holds the index of the file we want to play
int select_index = 0;
// holds how many wavs we found
int file_count = 0;
int loop_count = 0;

int lul_position = 0;
int play_pressed = 0;
int next_pressed = 0;
String playStatus = "STOP";

unsigned int curr_position = 0;
unsigned int clip_length = 0;
float played_perc = 0;
int shaft_position = 0;
float shaft_max = 18.0;


byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
};

byte lul[9] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B11111,
  B11111,
  B11011,
};

byte ball[8] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01110,
};

byte shaftt[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111
};

byte shaftb[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000
};

byte eikelt[8] = {
  B00000,
  B00000,
  B11000,
  B11100,
  B11110,
  B11110,
  B11111,
  B11111
};

byte eikelb[8] = {
  B11111,
  B11111,
  B11110,
  B11110,
  B11100,
  B11000,
  B00000,
  B00000
};

void getWavInfo() {
  // load all the file names into a char array

  gn_dir = SD.open("/");
  while(true){
    File entry =  gn_dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    if (entry.isDirectory()) {
      lcd.print("There's a directory on here...");
    } else {
      wavname = entry.name();

      wavnames[file_count] = wavname;

      String type = wavname.remove(4);
      if (type == "LOOP") {
        is_loop[file_count] = 1;
        loop_count += 1;
      }
      else {
        is_loop[file_count] = 0;
      }

      file_count++;
    }

  }
  lcd.setCursor(0, 3);
  delay(5000);
}

void selectNext() {
  // select the next audio file
  select_index++;
  if (select_index >= file_count) {
    select_index = 0;
  }
}

void printDirectory(File dir, int numTabs, int lcdrow) {
   while(true) {

     lcdrow++;
     if (lcdrow == 4) {
       lcdrow = 0;
     }
     lcd.setCursor(0, lcdrow);

     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     // for (uint8_t i=0; i<numTabs; i++) {
     //   lcd.print('\t');
     // }
     String thisnm = entry.name();
     lcd.print(thisnm.remove(20));
     // if (entry.isDirectory()) {
     //   lcd.print("/");
     //   printDirectory(entry, numTabs+1, lcdrow + 1);
     // } else {
       // files have sizes, directories do not
       // lcd.print("  ");
       // lcd.setCursor(0, lcdrow);
       // lcdrow++;
       // lcd.print(entry.size(), DEC);
     // }
     entry.close();
   }
}

void getPosition() {
  // clip_length = playWav1.lengthMillis();
  curr_position = playWav1.positionMillis();
  played_perc = ((float) curr_position) / ((float) clip_length);
  shaft_position = (int) (played_perc * shaft_max);
  // lcd.setCursor(10, 2);
  // lcd.print(shaft_position);
  Serial.println("new: ");
  Serial.println(curr_position);
  Serial.println((float) curr_position);
  Serial.println((float) clip_length);
  Serial.println(played_perc);
  Serial.println(shaft_position);
}

void printStatus() {
  // print currently playing etc.
  if (playWav1.isPlaying()) {
    playStatus = "PLAY";
  } else {
    playStatus = "STOP";
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selected:");
  lcd.setCursor(16, 0);
  lcd.print(playStatus);
  lcd.setCursor(0, 1);
  lcd.print(wavnames[select_index]);

  getPosition();
  lcd.setCursor(0, 2);
  lcd.write(byte(2));
  for (int i = 0; i <= shaft_position; i++) {
    lcd.write(byte(3));
  }
  lcd.write(byte(5));
  lcd.setCursor(0, 3);
  lcd.write(byte(2));
  for (int i = 0; i <= shaft_position; i++) {
    lcd.write(byte(4));
  }
  lcd.write(byte(6));

  delay(100);
}


void playFile(String str_name)
{
  str_len = str_name.length() + 1;
  str_name.toCharArray(name_arr, str_len);
  playWav1.play(name_arr);

  // A brief delay for the library read WAV info
  delay(5);
  clip_length = playWav1.lengthMillis();

  while (playWav1.isPlaying()) {
    // uncomment these lines if you audio shield
    // has the optional volume pot soldered
    // delay(200);
    // get button input
    b3_val = digitalRead(b3);
    if (b3_val == LOW) {
      // lcd.setCursor(16, 0);
      // lcd.write("STOP");
      play_pressed = 0;
      playWav1.stop();
      playWav1.play(name_arr);
      delay(200);
    }
    b4_val = digitalRead(b4);
    if (b4_val == LOW) {
      playWav1.stop();
      selectNext();
      delay(300);
    }
    printStatus();
  }

}

void setup() {
  // set up buttons
  pinMode(b2, INPUT);

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  // Print a message to the LCD.
  // lcd.print("hello, world!");
  // lcd.autoscroll();
  lcd.noBlink();

  // set up audio stuff
  SPI.setMOSI(SDCARD_MOSI_PIN);  // Audio shield has MOSI on pin 7
  SPI.setSCK(SDCARD_SCK_PIN);  // Audio shield has SCK on pin 14

  lcd.print("Initializing SD...");
  lcd.setCursor(0,1);

  if (!SD.begin(chipSelect)) {
    lcd.println("can't read SD :(");
    return;
  }
  root = SD.open("/");


  lcd.print("done! :)");
  // lcd.setCursor(0, 2);
  // lcd.print("printing file names");
  // delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  // printDirectory(root, 0, 0);
  // audio setup
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  // get all the wav names and lengths
  getWavInfo();
  lcd.createChar(0, smiley);
  lcd.createChar(1, lul);
  lcd.createChar(2, ball);
  lcd.createChar(3, shaftt);
  lcd.createChar(4, shaftb);
  lcd.createChar(5, eikelt);
  lcd.createChar(6, eikelb);
  Serial.begin(9600);
  Serial.println("--- Start Serial Monitor SEND_RCVE ---");
  Serial.println(" Type in Box above, . ");
  Serial.println("(Decimal)(Hex)(Character)");
  Serial.println();
}

void loop() {

  b3_val = digitalRead(b3);
  if (b3_val == LOW) {
    play_pressed = 1;
  }
  if (play_pressed == 1) {
    playFile(wavnames[select_index]);
  }

  b4_val = digitalRead(b4);
  if (b4_val == LOW) {
    // next_pressed = 1;
    selectNext();
    delay(300);
  }
  printStatus();

  // selectNext
}
