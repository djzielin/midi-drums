#ifndef DJZ_WAVE_GENERATOR
#define DJZ_WAVE_GENERATOR

#define TAU 6.283185307179586 //2*M_PI

class wave_generator //TODO support more waveforms (and also blend between square and sine)
{
public:
   wave_generator(float sample_rate);
   void set_speed(float hz);
   float tick();
   float sq_tick();
   void reset();

private:
   float _sample_rate;
   float _speed;
   float _inc_amount;
   float _t;
};

#endif
