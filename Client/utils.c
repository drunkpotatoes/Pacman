#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

void mem_err (char * variable)
{
  
  fprintf (stderr, "Error allocating memory for %s...\n", variable);
  fprintf (stderr, "Exiting program...\n");
  exit(1);
}
