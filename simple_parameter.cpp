#include "simple_parameter.h"
#include <stdio.h>
#include <iostream>

simple_parameter::simple_parameter(std::string name, float initial_value=0, float g_min=0, float g_max=1.0)
{
   _g_min=g_min;
   _g_max=g_max;

   _name=name;
   set_value(initial_value);
   printf("setting up: %s\n",name.c_str());
}

void simple_parameter::write_config(Setting &effects)
{
   Setting &single_effect = effects.add(Setting::TypeGroup);
   single_effect.add("name", Setting::TypeString) = _name;
   single_effect.add("min",Setting::TypeFloat) = _g_min;
   single_effect.add("max",Setting::TypeFloat) = _g_max;
   single_effect.add("value",Setting::TypeFloat) = _value;
}

simple_parameter::simple_parameter(Setting &effect)
{
   string sval;
   float  fval;

   if(effect.lookupValue("name", sval))
   {
      _name=sval;
      cout << "found effect name: " << _name << endl;
   }
   
   if(effect.lookupValue("min", fval))
   {
      _g_min=fval;
      cout << "found effect min: " << _g_min << endl;
   }
   if(effect.lookupValue("max", fval))
   {
      _g_max=fval;
      cout << "found effect max: " << _g_max << endl;
   }  
   if(effect.lookupValue("value", fval))
   {
      _value=fval;
      cout << "found effect value: " << _value << endl;
   }
}

float simple_parameter::get_value()
{
   return _value;
}

bool simple_parameter::get_bool()
{
   if(_value==0.0)
      return false;
   
   return true;
}

void simple_parameter::set_value(float value)
{ 
   _value=value; 
   //TODO, set GUI display of value here?
}

static void sp_callback(Fl_Widget* o, void *receiver)
{
   float value=((Fl_Valuator*)o)->value();
   printf("%s: %f     \r", o->label(), value);
   fflush(stdout);

   simple_parameter *sp=(simple_parameter *)receiver;
   sp->set_value(value);
}

void simple_parameter::init_gui()
{
   char *name=new char[100];
   sprintf(name,"%s",_name.c_str());
 
   _vs= new Fl_Value_Slider(140, 370, 130, 20, name);
   _vs->type(5);
   _vs->box(FL_FLAT_BOX);
   _vs->color((Fl_Color)10);
   _vs->selection_color((Fl_Color)1);
   _vs->labelsize(8);
   _vs->precision(3);
   _vs->bounds(_g_min,_g_max);
   _vs->value(_value);
   _vs->callback((Fl_Callback*)sp_callback,this); 
}


