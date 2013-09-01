#include "itkDisSimMorphologicalWatershedFromMarkersImageFilter.h"
#include <iostream>
#include "ioutils.h"
#include <itkNumericTraits.h>

template< class TInput1, class TOutput = TInput1 >
class DifPriority
{
public:
  DifPriority() {}
  ~DifPriority() {}
  bool operator!=(const DifPriority &) const
  {
    return false;
  }

  bool operator==(const DifPriority & other) const
  {
    return !( *this != other );
  }

  // A is the centre pixel, B the neighbour
  inline TOutput operator()(const TInput1 & A, const TInput1 & B) const
  {
    return static_cast< TOutput > (vcl_abs( B - A ));
  }
};


int main(int, char * argv[])
{
  const int dimension=2;

  typedef itk::Image<unsigned char, dimension> LabImType;
  typedef itk::Image<short, dimension> RawImType;

  typedef itk::DisSimMorphologicalWatershedFromMarkersImageFilter<RawImType, LabImType, DifPriority < RawImType::PixelType, itk::NumericTraits<RawImType::PixelType>::RealType> > WSType;

  RawImType::Pointer control = readIm<RawImType>(argv[1]);
  LabImType::Pointer marker = readIm<LabImType>(argv[2]);

  WSType::Pointer WS = WSType::New();

  WS->SetInput(control);
  WS->SetMarkerImage(marker);
  WS->SetMarkWatershedLine(false);
  writeIm<LabImType>(WS->GetOutput(), argv[3]);

  return(EXIT_SUCCESS);
}
