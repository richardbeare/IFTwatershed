#include "itkIFTWatershedFromMarkersImageFilter.h"
#include <iostream>


int main(int, char * argv[])
{
  const int dimension=2;

  typedef itk::Image<unsigned char, dimension> LabImType;
  typedef itk::Image<short, dimension> RawImType;

  typedef itk::IFTWatershedFromMarkersImageFilter<RawImType, LabImType> IFTWSType;

  IFTWSType::Pointer IFT = IFTWSType::New();

  return(EXIT_SUCCESS);
}
