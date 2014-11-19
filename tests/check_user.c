#include <assert.h>
#include "ythtbbs.h"

int main()
{
  assert(sizeof(struct userec) == 128);
  return 0;
}
