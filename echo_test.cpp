#include<stdio.h>
#include<stdlib.h>
#include "echo_effect.h"

void target_test(int t) //TODO: make sure values are valid at each step
{
   echo_effect *ee=new echo_effect(20,2);

   ee->set_target_length(t);
   ee->set_read_speed(2);

   for(int i=0;i<50;i++)
     ee->tick(0);

   float target=ee->get_delay_line_length();

   if((int)target==t)
   {
      printf("   test passed\n");
   }
   else
   {
      printf("test failed. wanted %d got %f\n",t,target); 
      exit(1);
   }
   
   delete(ee);
}


void min_test() //TODO check values to see if they are valid
{
   echo_effect *ee=new echo_effect(20,2);

   ee->set_target_length(-1);
   ee->set_read_speed(2);

   for(int i=0;i<50;i++)
     ee->tick(0);

   float target=ee->get_delay_line_length();

   if((int)target==0)
   {
      printf("   test passed\n");
   }
   else
   {
      printf("test failed. wanted 0 got %f\n",target); 
      exit(1);
   }
   
   delete(ee);
}


void max_test()
{
   echo_effect *ee=new echo_effect(20,2);

   ee->set_target_length(-1);
   ee->set_read_speed(-2);

   for(int i=0;i<50;i++)
     ee->tick(0);

   float target=ee->get_delay_line_length();

   int goal=20*2-1;
   if((int)target==goal) //TODO, should be based on parameters
   {
      printf("   test passed\n");
   }
   else
   {
      printf("test failed. wanted %d got %f\n",goal,target); 
      exit(1);
   }
   
   delete(ee);
}


int main(int argc, char *argv[])
{
   //target_test(5);
   //min_test();
   max_test();

   return 0;
}
