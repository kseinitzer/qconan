#include <iostream>
#include "libA.h"

int main() 
{
  if (LibA::sum(1, 1) == 2)
    return 0;
  else
    return -1;

}
