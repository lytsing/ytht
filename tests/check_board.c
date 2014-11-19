#include <assert.h>
#include "ythtbbs.h"

int main()
{
  assert(sizeof(struct boardheader) == 512);
  return 0;
}
