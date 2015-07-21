#ifndef DJZ_echo_effect
#define DJZ_echo_effect

class echo_effect
{
public:
   echo_effect(float sample_rate, float max_delay_seconds);
   ~echo_effect();

   //void set_lfo_depth(float depth);
   //void set_lfo_freq(float freq);
   void set_delay_time(float dt);
   void set_feedback(float f);
   void set_effect_mix(float em);
   
   float tick(float input);
   //float tick_trails(float input);
   void set_active(bool value) { _is_active=value; }
   float get_delay_line_length();
   void set_target_length(int length);
   
   void set_read_speed(float val);
   
   void force_length(int l);

private:
   //wave_generator *_wg;

   bool _is_active;
   
   int _delay_line_max;
   float *_delay_line;

   //float _lfo_freq;
   //float _lfo_depth;
   //float _original_freq;

   float _sample_rate;
   //unsigned int   _current_delay;
   float _feedback;
   float _effect_mix;

   float _read_pos;
   int  _write_pos;

   float _read_speed;
   int _delay_target;
   //int _stored_original_size;
   //int _stored_write_head;
   //bool _reached_target;

 //  bool _has_lfo;
  
};


#endif
