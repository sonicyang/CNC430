#include <Stepper.h>
#include <String.h>
#include <Stream.h>

#define STEPS 100
#define EN 5 //Distance to place pen
#define mul 50

//X max 220
//y max 250

Stepper stepperx(STEPS, 6, 7, 8, 9); // X Axis Stepper initialize
Stepper steppery(STEPS, 10, 11, 12, 13); // Y Axis Stepper initialize
Stepper stepperz(STEPS, 14, 15, 18, 19); // Y Axis Stepper initialize

int x_pos = 0; //current x position
int y_pos = 0; //current y position
int z_pos = 0; //current z position
int feed = 30;

int sofar;
char buffer[128];
boolean mode_abs = true;

void setup()
{
  Serial.begin(9600); // G-Code receiver 
  pinMode(EN, OUTPUT);  //EN ctl
  
  Enable(true);
  feedrate(50);
  
  //help();
  prompt();
}

void loop()
{
  while(Serial.available() > 0) {  // if something is available
    char c=Serial.read();  // get it
   // Serial.print(c);  // repeat it back so I know you got the message
    if(sofar<64) buffer[sofar++]=c;  // store it
    if(buffer[sofar-1]=='\n') break;  // entire message received
  }
  if(sofar>0 && buffer[sofar-1]=='\n') {
    // we got a message and it ends with a semicolon
    buffer[sofar]=0;  // end the buffer so string functions work right
    //Serial.print(buffer);
    //Serial.print("\n");
    processCommand();  // do something with the command
    Serial.print("A");
    prompt();
  }
}

/*
  command processing
*/
void processCommand(){
  
    int cmd;
    float paralist[5] = {0,0,0,-1,0};
    int paranum = 0;
    
    char * ptr = strtok(buffer," "); //Split String
    cmd = atoi(ptr+1);             //Convert command
    if(mode_abs){
      paralist[0] = x_pos;
      paralist[1] = y_pos;
      paralist[2] = z_pos;
    }
    ptr = strtok(NULL," ");        //Skip the G/M command
    while(ptr != NULL){
      switch(*ptr){
        case 'X':
          paralist[0] = atof(ptr+1);
          paralist[0] = floor(paralist[0]*mul+0.5);
          break;
        case 'Y':
          paralist[1] = atof(ptr+1);
          paralist[1] = floor(paralist[1]*mul+0.5);
          break;
        case 'Z':
          paralist[2] = atof(ptr+1);
          paralist[2] = floor(paralist[2]+0.5);
          break;
        case 'F':
          paralist[3] = atof(ptr+1);
          paralist[3] = floor(paralist[3]+0.5);
          break;
        case 'P':
          paralist[4] = atoi(ptr+1);
          break;
      }
      ptr = strtok(NULL," ");
      paranum++;
    }
  
  // look for commands that start with 'G'
  if(buffer[0] == 'G'){   
    switch(cmd) {
      case  0: // move in a line
      case  1: // move in a line
        if(paralist[3] >= 0)
          feedrate(paralist[3]);
        if(mode_abs){
          movea(paralist[0],paralist[1]);
          headera(paralist[2]);
        }else{
          mover(paralist[0],paralist[1]);
          headerr(paralist[2]);
        }
        break;
      // case  2: // clockwise arc
      // case  3: // counter-clockwise arc
      case  4:  delay(paralist[4]);  break;  // wait a while
      case 90:  mode_abs=1;  break;  // absolute mode
      case 91:  mode_abs=0;  break;  // relative mode
      case 92:  // set logical position
        reset();
        movea(0,0);
        break;
      default:  break;
    }
  }else if(buffer[0] == 'M'){
    switch(cmd) {
      case 17:  // turns off power to steppers (releases the grip)
        Enable(true);
        break;
      case 18:  // turns off power to steppers (releases the grip)
        Enable(false);
        break;
      case 100:  help();  break;
      case 114:  where();  break;  // prints px, py, fr, and mode.
      default:  break;
    }
  }
}

/**
 * display CNC information
 */
void where() {
  Serial.println(F("Current State:"));
  Serial.print(F("X:")); 
  Serial.println(x_pos);
  Serial.print(F("Y:")); 
  Serial.println(y_pos);
  Serial.print(F("Z:")); 
  Serial.println(z_pos);
  Serial.print(F("FeedRate:")); 
  Serial.println(feed);
}

/**
 * display helpful information
 */
void help() {
  Serial.println(F("Commands:"));
  Serial.println(F("G00 [X(steps)] [Y(steps)] [Z(steps)] [F(feedrate)]; - linear move"));
  Serial.println(F("G01 [X(steps)] [Y(steps)] [Z(steps)] [F(feedrate)]; - linear move"));
  Serial.println(F("G04 P[seconds]; - delay"));
  Serial.println(F("G90; - absolute mode"));
  Serial.println(F("G91; - relative mode"));
  Serial.println(F("G92 [X(steps)] [Y(steps)]; - change logical position"));
  Serial.println(F("M18; - disable motors"));
  Serial.println(F("M100; - this help message"));
  Serial.println(F("M114; - report position and feedrate"));
}

/*
  Reset Buffer and prompt
*/
void prompt(){
  sofar=0;  // clear input buffer
  //Serial.print(F(">"));  // signal ready to receive input
}

/*
  Enable Output?
*/
void Enable(boolean x){
  if(x)
    digitalWrite(EN,HIGH);
  else
    digitalWrite(EN,LOW);
}

/*
  Set speed function
*/
void feedrate(int rate){
  feed = rate;
  stepperx.setSpeed(feed); //Set X Axis Stepper Motor speeds
  steppery.setSpeed(feed); //Set Y Axis Stepper Motor speeds
  stepperz.setSpeed(feed); //Set Y Axis Stepper Motor speeds
}

/*
  Reset header position 
*/
void reset(){
  headerr((-1)*z_pos);
}

/*
  Absolute header movement 
*/
void headera(int z){
  int dz = z - z_pos;
  headerr(dz);
}

/*
  Relative header movement 
*/
void headerr(int z){
  stepperz.step(z);
  z_pos += z;
}

/*
  Move X,Y to a Sepicified position(a Mesh up of mover routing, also using Bresenham Algorithm)
*/
void movea(int x,int y){
  int dx = x - x_pos;
  int dy = y - y_pos;
  mover(dx,dy);
}

/*
  Move X,Y in relative mode(using Bresenham Algorithm)
*/
void mover(int x,int y){
  short dirx = x / abs(x);
  short diry = y / abs(y);
  x = abs(x);
  y = abs(y);
  if(x == y){
    for(int i = 0; i < x; i++){
      stepperx.step(dirx);
      x_pos += dirx;
      steppery.step(diry);
      y_pos += diry;
    }    
  }else if(x > y){
    float acc = 0;
    boolean flag = false;
    for(int i = 0; i < x; i++){
      stepperx.step(dirx);
      x_pos += dirx;
      if(flag){
        steppery.step(diry);
        y_pos += diry;
        flag = false;
      }
      acc += (float)y / x;
      if(acc > 0.5){
        flag = true;
        acc--;
      }
    }    
  }else{
    float acc = 0;
    boolean flag = false;
    for(int i = 0; i < y; i++){
      steppery.step(diry);
      y_pos += diry;
      if(flag){
        stepperx.step(dirx);
        x_pos += dirx;
        flag = false;
      }
      acc += (float)x / y;
      if(acc > 0.5){
        flag = true;
        acc--;
      }
    }
  }
}

float atof(char * ptr){
  float x = 0;
  float i = 10;
  boolean flag = false;
  boolean m = false;
  while(*ptr != '\0'){
    if(flag){
      if(*ptr < 48 || *ptr > 57){
        ptr++;
        continue;
      }
      x += (*ptr - 48) / i;
      i*=10;
    }else{
      if(*ptr == 46)
        flag = true;
      if(*ptr == 45)
        m = true;
      if(*ptr < 48 || *ptr > 57){
        ptr++;
        continue;
      }
      x *= 10;
      x += (*ptr - 48);
    }
    ptr++;
  }
  if(m)
    x *= (-1);
  return x;
}
