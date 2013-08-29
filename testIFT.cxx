#include "itkIFTWatershedFromMarkersImageFilter.h"
#include <iostream>
#include "ioutils.h"

int main(int, char * argv[])
{
  const int dimension=2;

  typedef itk::Image<unsigned char, dimension> LabImType;
  typedef itk::Image<short, dimension> RawImType;

  typedef itk::IFTWatershedFromMarkersImageFilter<RawImType, LabImType> IFTWSType;

  RawImType::Pointer control = readIm<RawImType>(argv[1]);
  LabImType::Pointer marker = readIm<LabImType>(argv[2]);

  IFTWSType::Pointer IFT = IFTWSType::New();

  IFT->SetInput(control);
  IFT->SetMarkerImage(marker);

  writeIm<LabImType>(IFT->GetOutput(), argv[3]);

  return(EXIT_SUCCESS);
}
