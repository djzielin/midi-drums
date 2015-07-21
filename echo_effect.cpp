#include "echo_effect.h"
#include <stdio.h>

echo_effect::~echo_effect()
{
   printf("in destructor\n");
   delete _delay_line;

}

echo_effect::echo_effect(float sample_rate, float max_delay_seconds)
{
   printf("initialing delay: max %f seconds\n",max_delay_seconds);

   _sample_rate=sample_rate;
   _effect_mix=1.0;

   _delay_line_max=max_delay_seconds*sample_rate;
   printf("number of samples in delay line: %d\n",_delay_line_max);
   _delay_line=new float[_delay_line_max];

   _read_pos=0;
   _write_pos=_delay_line_max/2;

   for(int i=0;i<_delay_line_max;i++)
      _delay_line[i]=0.0f;

   //_wg=new wave_generator(_sample_rate);

   //_reached_target=true;
   set_feedback(0.5);
   set_read_speed(1);
   set_target_length(-1);
   force_length(sample_rate*max_delay_seconds*0.5);

   printf("   DONE!\n");
}

float echo_effect::get_delay_line_length()
{
   if(_write_pos>=_read_pos)
      return (float)_write_pos-_read_pos;
   else
      return (float)_delay_line_max-_read_pos+_write_pos;
}

void echo_effect::set_read_speed(float val)
{
    //printf("read speed: %f\n",val);

    _read_speed=val;
}

void echo_effect::set_target_length(int length)
{
   if(length!=-1)
   {
      if(length>_delay_line_max-1) 
         length=_delay_line_max-1;

   }

   _delay_target=length;
}

void echo_effect::set_delay_time(float dt)
{
   int td=(int)(dt*_sample_rate);
   set_target_length(td);
  
   //printf("target delay is now: %d\n",_delay_target);

   float rspeed=get_delay_line_length()/_delay_target;
   set_read_speed(rspeed);
}

void echo_effect::set_feedback(float f)    
{
   _feedback=f;
}

void echo_effect::set_effect_mix(float em)
{
   _effect_mix=em;
} 

void echo_effect::force_length(int l)
{
   _read_pos=_write_pos-l;
   
   if(_read_pos>_delay_line_max) //wrap around going forward
      _read_pos-=_delay_line_max;
   if(_read_pos<0)               //wrap around goind backward
      _read_pos+=_delay_line_max;
}

float echo_effect::tick(float input)
{
   float read_val=_delay_line[(int)_read_pos]; //TODO: interpolate
   _delay_line[_write_pos]=input+read_val*_feedback; 
  
   float read_unrolled;
   float old_read=_read_pos;
   

   _write_pos=(_write_pos+1) % _delay_line_max;
   //printf("read pos (pre clamping): %f\n",_read_pos);
   //printf("write pos: %d\n",_write_pos);

   //printf("read pos pre unrolling: %f\n",_read_pos);
   if(_read_pos<_write_pos)
      ;//read_unrolled=_read_pos;
   else
      _read_pos-=_delay_line_max;

   //printf("read post unrolling: %f\n",_read_pos);
   

   _read_pos+=_read_speed; 
   //printf("read post add: %f\n",_read_pos);
 
   

   if(_read_pos>_write_pos) //limit read from going past write
   {
      _read_pos=_write_pos;
      printf("limit buffer getting too short\n");
   }

   int len=_write_pos-_read_pos;
   if(_delay_target!=-1 && len<_delay_target) //limit to target
   {
      _read_pos=(float)(_write_pos-_delay_target); 
      printf("limit to target\n");
      //printf("  delay target: %d\n",_delay_target);
      //printf("  old length was: %d\n",len);
      //printf("  write is: %d\n",_write_pos);
      //printf("  new read is: %f\n",_read_pos);
   }
   if(len>_delay_line_max-1) // limit from getting buffer to long
   {
      _read_pos=(float)(_write_pos-_delay_line_max+1.0);
      printf("limit from getting buffer too long\n");
      //printf("  write pos: %d\n",_write_pos);
      //printf("  delay line max: %d\n",_delay_line_max);
      
   }
   
   if(_read_pos>_delay_line_max) //wrap around going forward
      _read_pos-=_delay_line_max;
   if(_read_pos<0)               //wrap around goind backward
      _read_pos+=_delay_line_max;
      
   //printf("read pos: %f\n",_read_pos);
   //printf("R: %.02f W: %02d Length: %.02f Read Val: %.02f read speed: %.02f\n",_read_pos,_write_pos,get_delay_line_length(),read_val,_read_speed);

   return read_val*_effect_mix; //+input*(1-_effect_mix); //make signal return all wet

 // return 0;
}


/*
void echo_effect::set_lfo_depth(float depth)
{
   _lfo_depth=depth*0.1;
   printf("setting lfo depth to: %f\n",_lfo_depth); 
}
     
void echo_effect::set_lfo_freq(float freq)
{
   _lfo_freq=freq*1;
   printf("lfo freq will be: %f\n",_lfo_freq);
   _wg->set_speed(_lfo_freq);
}
*/
