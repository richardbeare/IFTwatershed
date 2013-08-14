#include "itkIFTQueue.h"

typedef long IndexType;
typedef float PriorityType;
typedef int IterationType;

typedef class {
  PriorityType P;
  IterationType time;
} CombPriorityType;


typedef IFTQueueA<CombPriorityType, IndexType> ThisQueueType;

int main(int, char * argv[])
{

  ThisQueueType Q;

  return(EXIT_SUCCESS);
}
