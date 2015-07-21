#include "single_parameter.h"
#include <stdio.h>
#include <iostream>

single_parameter::single_parameter(Setting &effect)
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
   if(effect.lookupValue("v_min", fval))
   {
      _v_min=fval;
      cout << "found effect min value: " << _v_min << endl;
   }
   if(effect.lookupValue("v_max", fval))
   {
      _v_max=fval;
      cout << "found effect max value: " << _v_max << endl;
   }

   set_min(_v_min);
   set_max(_v_max);
}

void single_parameter::write_config(Setting &effects)
{
   Setting &single_effect = effects.add(Setting::TypeGroup);
   single_effect.add("name", Setting::TypeString) = _name;
   single_effect.add("min",Setting::TypeFloat) = _g_min;
   single_effect.add("max",Setting::TypeFloat) = _g_max;
   single_effect.add("v_min",Setting::TypeFloat) = _v_min;
   single_effect.add("v_max",Setting::TypeFloat) = _v_max;
}


single_parameter::single_parameter(std::string name, float initial_min, float initial_max, float g_min, float g_max)
{
   _g_min=g_min;
   _g_max=g_max;
   _name=name;
   
   set_min(initial_min);
   set_max(initial_max);
 
   printf("setting up: %s\n",name.c_str());
}

float single_parameter::calc_value(float velocity)
{
   return _v_min+velocity*_v_delta;
}

void single_parameter::update_helper_vals()
{
   if(_v_min==0 && _v_max==0) _active=false;
   else                       _active=true;
  
   _v_delta=_v_max-_v_min;

   //cout << "_vdelta: " << _v_delta << " _v_min: " << _v_min << endl;
}

void single_parameter::set_max(float value)
{ 
   _v_max=value;
   update_helper_vals();
}

void single_parameter::set_min(float value)
{
   _v_min=value;
   update_helper_vals();
}

static void sp_max_callback(Fl_Widget* o, void *receiver)
{
   float value=((Fl_Valuator*)o)->value();
   printf("%s: %f     \r", o->label(), value);
   fflush(stdout);

   single_parameter *sp=(single_parameter *)receiver;
   sp->set_max(value);
}

static void sp_min_callback(Fl_Widget* o, void *receiver)
{
   float value=((Fl_Valuator*)o)->value();
   printf("%s: %f     \r", o->label(), value);
   fflush(stdout);

   single_parameter *sp=(single_parameter *)receiver;
   sp->set_min(value);
}

void single_parameter::init_gui()
{
   char *name=new char[100];
   sprintf(name,"%s: min",_name.c_str());
 
   _vs= new Fl_Value_Slider(140, 370, 130, 20, name);
   _vs->type(5);
   _vs->box(FL_FLAT_BOX);
   _vs->color((Fl_Color)10);
   _vs->selection_color((Fl_Color)1);
   _vs->labelsize(8);
   _vs->bounds(_g_min,_g_max);
   _vs->value(_v_min);
   _vs->callback((Fl_Callback*)sp_min_callback,this);
   
   char *name2=new char[100];
   sprintf(name2,"%s: max",_name.c_str());

   _vs2 = new Fl_Value_Slider(140, 370, 130, 20, name2);
   _vs2->type(5);
   _vs2->box(FL_FLAT_BOX);
   _vs2->color((Fl_Color)10);
   _vs2->selection_color((Fl_Color)1);
   _vs2->labelsize(8);
   _vs2->value(_v_max);
   _vs2->bounds(_g_min,_g_max);
   _vs2->callback((Fl_Callback*)sp_max_callback,this);
}



