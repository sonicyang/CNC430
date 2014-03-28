#include <Stepper.h>
#include <String.h>
#include <Stream.h>
#include <math.h>

#define STEPS 100
#define EN 5 //Distance to place pen
#define mul 4

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
  
  help();
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
    float paralist[7] = {0,0,0,-1,0,0,0};
    int paraexist = 0;
    
    if(buffer[0] == '(')
      return;
    
    char * ptr = strtok(buffer," "); //Split String
    cmd = atoi(ptr+1);             //Convert command
    if(mode_abs || cmd == '3' || cmd == '2'){
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
          paraexist &= 1;
          break;
        case 'Y':
          paralist[1] = atof(ptr+1);
          paralist[1] = floor(paralist[1]*mul+0.5);
          paraexist &= 2;
          break;
        case 'Z':
          paralist[2] = atof(ptr+1);
          paralist[2] = floor(paralist[2]*10+0.5);
          paraexist &= 4;
          break;
        case 'F':
          paralist[3] = atof(ptr+1);
          paralist[3] = floor(paralist[3]+0.5);
          break;
        case 'P':
          paralist[4] = atoi(ptr+1);
          break;
        case 'I':
          paralist[5] = atof(ptr+1);
          paralist[5] = floor(paralist[5]);
          break;
        case 'J':
          paralist[6] = atof(ptr+1);
          paralist[6] = floor(paralist[6]);
          break;
      }
      ptr = strtok(NULL," ");
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
      case  2:
      case  3:
        Circle(paralist[0],paralist[1],paralist[5],paralist[6],cmd - 2);
        break;
      case  4:  delay(paralist[4]);  break;  // wait a while
      case 28:  reset(); movea(0,0); break;
      case 90:  mode_abs = 1;  break;  // absolute mode
      case 91:  mode_abs = 0;  break;  // relative mode
      case 92:  // set logical position
        if(paraexist > 0){
          if((paraexist & 1) == 1){
            x_pos = paralist[0];
          }
          if((paraexist & 2) == 2){
            y_pos = paralist[1];
          }
          if((paraexist & 4) == 4){
            z_pos = paralist[2];
          }
        }else{
          x_pos = 0;
          y_pos = 0;
          z_pos = 0;
        }
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
  Serial.println(F("G92 [X(steps)] [Y(steps)] [Z(steps)]; - change logical position"));
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
  headerr(z_pos);
}

/*
  Absolute header movement 
*/
void headera(int z){
  int dz = z_pos - z;
  headerr(dz);
}

/*
  Relative header movement 
*/
void headerr(int z){
  stepperz.step(z * (-1));
  z_pos -= z;
}

/*
  Move X,Y to a Sepicified position(a Mesh up of mover routing, also using Bresenham Algorithm)
*/
void movea(float x,float y){
  int dx = x - x_pos;
  int dy = y - y_pos;
  mover(dx,dy);
}

/*
  Move X,Y in relative mode(using Bresenham Algorithm)
*/
void mover(float x,float y){
  short dirx = x / abs(x);
  short diry = y / abs(y);
  x = abs(x);
  y = abs(y);
  if(x == y){
    for(int i = 0; i < x; i++){
      stepperx.step(dirx * (-1));
      x_pos += dirx;
      steppery.step(diry);
      y_pos += diry;
    }
  }else if(x > y){
    float acc = 0;
    boolean flag = false;
    for(int i = 0; i < x; i++){
      stepperx.step(dirx * (-1));
      x_pos += dirx;
      if(flag){
        steppery.step(diry);
        y_pos += diry;
        flag = false;
      }
      acc += y / x;
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
        stepperx.step(dirx * (-1));
        x_pos += dirx;
        flag = false;
      }
      acc += x / y;
      if(acc > 0.5){
        flag = true;
        acc--;
      }
    }
  }
}


/*
  Draw X,Y in Circle mode(using Bresenham Algorithm)
*/
void Circle(float x,float y, float i, float j, int dir){
  if(i == j && j == 0)
    movea(x,y);
  
  
  float centx, centy;
  
  // Centre coordinates are always relative
  centx = i + x_pos/mul;
  centy = j + y_pos/mul;
  float angleA, angleB, angle, radius, length, aX, aY, bX, bY;
  
  aX = (x_pos/mul - centx);
  aY = (y_pos/mul - centy);
  bX = (x/mul - centx);
  bY = (y/mul - centy);
  
  if (dir == 0) { // Clockwise
  	angleA = atan2(bY, bX);
  	angleB = atan2(aY, aX);
  } else { // Counterclockwise
  	angleA = atan2(aY, aX);
  	angleB = atan2(bY, bX);
  }
  
  // Make sure angleB is always greater than angleA
  // and if not add 2PI so that it is (this also takes
  // care of the special case of angleA == angleB,
  // ie we want a complete circle)
  if (angleB <= angleA) angleB += 2 * M_PI;
  angle = angleB - angleA;
  if(angle == 0)
    angle = 2 * M_PI;
  
  radius = sqrt(aX * aX + aY * aY);
  length = radius * angle;
  int steps, s, ss;
  steps = (int) ceil(length / 0.1);
  
  /*
  Serial.print(x_pos/mul);
  Serial.print("\t");
  Serial.println(y_pos/mul);
  
  Serial.print(centx);
  Serial.print("\t");
  Serial.println(centy);
  
  Serial.print(x/mul);
  Serial.print("\t");
  Serial.println(y/mul);
  
  
  Serial.print(angleA);
  Serial.print("\t");
  Serial.println(angleB);
  
  Serial.println(radius);
  Serial.println(length);
  
  */
  float nx, ny;
  for (s = 1; s <= steps; s++) {
  	ss = (dir == 1) ? s : steps - s; // Work backwards for CW
  	nx = centx + radius * fastsin(angleA + angle * ((float) ss / steps) + (M_PI/2));
  	ny = centy + radius * fastsin(angleA + angle * ((float) ss / steps));
        Serial.print(nx);
        Serial.print("\t");
        Serial.println(ny);
  	movea(nx, ny);
        
  	// Need to calculate rate for each section of curve
  	/*if (feedrate > 0)
  		feedrate_micros = calculate_feedrate_delay(feedrate);
  	else
  		feedrate_micros = getMaxSpeed();*/
  
  	// Make step
  	//dda_move(feedrate_micros);
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

float hypot(float x, float y){
  return sqrt(x * x + y * y);
}

float fastsin(float x)
{
  while(x > M_PI)  
    x -= 2*M_PI;
  while(x < -M_PI)  
    x += 2*M_PI;
    
  float B = (4 / M_PI);
  float c = -4 / (M_PI * M_PI);

  //return 0;
  return B * x + c * x * ((x < 0) ? -x : x);
} 


float tsin(float x){
  return x - (x*x*x / 6) + (x*x*x*x*x / 120) - (x*x*x*x*x*x*x / 5040) + (x*x*x*x*x*x*x*x*x / 362880);
}

float tcos(float x){
  return x*x - (x*x*x*x / 2) + (x*x*x*x*x*x / 24) - (x*x*x*x*x*x*x*x / 720) + (x*x*x*x*x*x*x*x*x*x / 40320);
}

float tasin(float x){
  return x + (x*x*x /6) + ((3/40)*x*x*x*x*x) + ((5/112)*x*x*x*x*x*x*x);
}

float tatan2(float y, float x){
  float z = y / x;
  return z - (z*z*z / 3) + (z*z*z*z*z / 5) - (z*z*z*z*z*z*z / 7) + (z*z*z*z*z*z*z*z*z / 9);
}

