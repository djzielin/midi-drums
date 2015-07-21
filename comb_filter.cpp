#include "comb_filter.h"
#include <stdio.h>

comb_filter::comb_filter(float sample_rate, float max_delay_seconds)
{
   printf("initialing delay: max %f seconds\n",max_delay_seconds);

   _read_pos=0;
   _write_pos=0;

   _sample_rate=sample_rate;
   _effect_mix=1.0;

   _wg=new wave_generator(_sample_rate);

   set_delay_time(0.1);
   set_feedback(0.5);
   set_lfo_freq(0);
   set_lfo_depth(0); 
 
   _delay_line_max=max_delay_seconds*sample_rate;
   _delay_line=new float[_delay_line_max];

   for(int i=0;i<_delay_line_max;i++)
      _delay_line[i]=0.0f;
 


   printf("   DONE!\n");
}

void comb_filter::set_lfo_depth(float depth)
{
   _lfo_depth=depth*0.1;
   printf("setting lfo depth to: %f\n",_lfo_depth); 
}
     
void comb_filter::set_lfo_freq(float freq)
{
   _lfo_freq=freq*1;
   printf("lfo freq will be: %f\n",_lfo_freq);
   _wg->set_speed(_lfo_freq);
}

void comb_filter::set_delay_time(float dt)
{
   int new_delay=dt*_sample_rate;
   if(new_delay==_current_delay) return;
   
   _current_delay=new_delay;

   if(_current_delay<1) _current_delay=1;
   if(_current_delay>=_delay_line_max) _current_delay=_delay_line_max-1;
}

void comb_filter::set_feedback(float f)    
{
   _feedback=f;
}

void comb_filter::set_effect_mix(float em)
{
   _effect_mix=em;
} 

float comb_filter::tick(float input)
{
   float read_val=_delay_line[_read_pos];
   _delay_line[_write_pos]=input+read_val*_feedback;;

   _read_pos=(_read_pos+1) % _current_delay;
   _write_pos=(_write_pos+1) % _current_delay;

   //printf("effect mix: %f\n",_effect_mix);
   //printf("read_val: %f\n",read_val);

   return read_val*_effect_mix+input*(1-_effect_mix);

 // return 0;
}

