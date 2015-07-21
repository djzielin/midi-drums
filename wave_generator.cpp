#include <math.h>
#include "wave_generator.h"

wave_generator::wave_generator(float sample_rate)
{
   _sample_rate=sample_rate;
   reset();
   set_speed(440);
}

void wave_generator::reset()
{
   _t=0;
}

void wave_generator::set_speed(float hz)
{
   _speed=hz;
   _inc_amount=TAU*_speed*1.0/_sample_rate;
}

float wave_generator::tick()
{
   float val=(1.0f+sin(_t))*0.5f; //0 to 1

   _t+=_inc_amount;  
   if(_t>TAU) _t-=TAU;

   return val;
}

float wave_generator::sq_tick()
{
   float val=tick();

   if(val>0.5) val=1.0;
   else        val=0.0;

   return val;
}



