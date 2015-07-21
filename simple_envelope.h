#ifndef SINGLE_ENVELOPE_DJZ
#define SINGLE_ENVELOPE_DJZ

class simple_envelope
{
public:
   simple_envelope();
   void reset();
   float tick();
   void set_envelope(float d);
   int get_state() { return _envelope_state; }
   float get_decay_time() { return _decay_time; }
private:
   float _elapsed_samples;
   int   _envelope_state;
   float _decay_time; 
};

#endif
