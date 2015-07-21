#include "simple_envelope.h"
#include <stdio.h>

simple_envelope::simple_envelope()
{ 
   set_envelope(1000);
   reset();
}

void simple_envelope::reset()
{
   _elapsed_samples=0;
   _envelope_state=0;
}

float simple_envelope::tick()
{
   float value=0;

   if(_envelope_state==0)
   {
      value=(1.0-_elapsed_samples/_decay_time);
      _elapsed_samples++;
     
      if(_elapsed_samples>_decay_time) 
         _envelope_state++;
   } 

   return value;
}

void simple_envelope::set_envelope(float d)
{
   _decay_time=d;

   if(_decay_time==0) _decay_time=1.0;
}

