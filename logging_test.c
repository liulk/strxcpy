#include "logging.h"

int main()
{
  int i;
  for (i = 0; i < 5; i++)
    INFO("i = %d", i);

  DEBUG_EXPR("%d", i);

  return 0;
}
