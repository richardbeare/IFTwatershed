#include "itkIFTQueue.h"
#include <iostream>

typedef long IndexType;
typedef float PriorityType;
typedef int IterationType;

typedef class CombPriorityType{
public:
  PriorityType P;
  IterationType time;

} CombPriorityType;

std::ostream & operator<<(std::ostream & os, const CombPriorityType &P)
  {
  os << "[";
  os << "P=" << P.P << " t=" << P.time;
  os << "]";
  return os;
  }

class ComparePriority {
public:
  ComparePriority(){}
  ~ComparePriority(){}
  bool operator !=(const ComparePriority &) const
  {
    return(false);
  }

  bool operator==(const ComparePriority & other) const
  {
    return !( *this != other );
  }
  inline bool operator()(const CombPriorityType & A, const CombPriorityType & B) const
  {
    if (A.P < B.P) return true;
    if (A.P==B.P) return (A.time < B.time);
    return(false);
  }

 

};
typedef IFTQueueA<CombPriorityType, IndexType, ComparePriority> ThisQueueType;

int main(int, char * argv[])
{

  ThisQueueType Q;

  for (int t = 0; t < 10; t++)
    {
    CombPriorityType p;
    p.time=t;
    p.P = 5;
    // pretend t is an index too.
    Q.insert(t,p);
    }
  for (int t = 10; t < 20; t++)
    {
    CombPriorityType p;
    p.time=t;
    p.P = 3;
    // pretend t is an index too.
    Q.insert(t,p);
    }
  CombPriorityType f1 = Q.front_key();
  IndexType i1 = Q.front_value();

  std::cout << "priority=" << f1.P << " " << f1.time << " " << i1 << std::endl;
  std::cout << Q.size() << std::endl;

  Q.PrintKeyMap();
  std::cout << "===============" << std::endl;
  // insert an existing value with a new priority
  CombPriorityType p;
  p.P=10;p.time=29;

  Q.insert(10, p);
  
  Q.PrintKeyMap();
  std::cout << "===============" << std::endl;

  Q.pop();
  Q.PrintKeyMap();

  return(EXIT_SUCCESS);
}
