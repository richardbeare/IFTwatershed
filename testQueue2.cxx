#include "itkIFTQueue.h"
#include <iostream>

typedef long IndexType;
typedef float PriorityType;
typedef int IterationType;


typedef IFTQueueB<PriorityType, IndexType> ThisQueueType;

int main(int, char * argv[])
{

  ThisQueueType Q;

  for (int t = 0; t < 10; t++)
    {
    // pretend t is an index too.
    Q.insert(t,5);
    }
  for (int t = 10; t < 20; t++)
    {
    // pretend t is an index too.
    Q.insert(t,3);
    }
  PriorityType f1 = Q.front_key();
  IndexType i1 = Q.front_value();

  std::cout << "priority=" << f1 <<" " << i1 << std::endl;
  std::cout << Q.size() << std::endl;

  Q.PrintKeyMap();
  std::cout << "===============" << std::endl;
  // insert an existing value with a new priority
  PriorityType p;
  p=7;
  Q.insert(10, p);
  
  p=7;
  Q.insert(15, p);

  Q.PrintKeyMap();

  return(EXIT_SUCCESS);
}
