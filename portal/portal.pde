//libraries for serial and video;
import processing.serial.*;
import processing.video.*;

Serial myPort;
Movie myMovie;
int timeline=0;
int playbackJump=120;
String val;

//counter to keep track of jumps;
int counter = 0;

//timestams for transitions in the video;
int stamp1 = 97;
int stamp2 = 240;
int stamp3 = 357;
int stamp4 = 476;

void setup() {
  size(810, 350);
  fullScreen();
  String portName = Serial.list()[1];
  myPort = new Serial(this, portName, 9600);
  myMovie = new Movie(this, "video.mp4");
  myMovie.loop();
}
int[] nums;
int trigger;
void draw()
{
  if ( myPort.available() > 0)
  {
    val = myPort.readStringUntil('\n');
    nums = int(split(val, ','));
    if (nums[0]==1)
    {
      trigger = 1;
      nums[0]=3;
    }
  }
  if (trigger ==1)
  {
    counter =counter+1;
    //switch to timestamps based on counter;
    switch (counter)
    {
    case 1 :
      timeline = stamp1;
      break;
    case 2 :
      timeline = stamp2;
      break;
    case 3 :
      timeline = stamp3;
      break;
    case 4 :
      timeline = stamp4;
      break;
    default :
      counter = 0;
      break;
    }
    println(timeline);
    myMovie.jump(timeline);
    trigger = 0;
  }
  image(myMovie, 0, 0);
}
void movieEvent(Movie m) {
  m.read();
}
