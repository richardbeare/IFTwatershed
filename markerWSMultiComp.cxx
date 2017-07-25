// Dissimilarity transform using possibly multicomponent input images.
#include <itkMorphologicalWatershedFromMarkersImageFilter.h>
#include <itkMorphologicalGradientImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkOrientImageFilter.h>
#include "tclap/CmdLine.h"
#include "ioutils.h"

#include "itkDisSimMorphologicalWatershedFromMarkersImageFilter.h"

#ifdef USEPARA
#include <itkParabolicErodeImageFilter.h>
#include <itkParabolicDilateImageFilter.h>
#endif

typedef class CmdLineType
{
public:
  std::string InputIm, OutputIm, MarkerIm;
  bool MarkWSLine;
} CmdLineType;

void ParseCmdLine(int argc, char* argv[],
                  CmdLineType &CmdLineObj
                  )
{
  using namespace TCLAP;
  try
    {
    // Define the command line object.
    CmdLine cmd("markerWSMultiComp ", ' ', "0.9");

    ValueArg<std::string> inArg("i","input","input image",true,"result","string");
    cmd.add( inArg );

    ValueArg<std::string> markArg("m","marker","marker image",true,"result","string");
    cmd.add( markArg );

    ValueArg<std::string> outArg("o","output","output image", true,"","string");
    cmd.add( outArg );


    SwitchArg lineArg("l","markline","Mark the watershed line", false);
    cmd.add(lineArg);


    // Parse the args.
    cmd.parse( argc, argv );

    CmdLineObj.InputIm = inArg.getValue();
    CmdLineObj.OutputIm = outArg.getValue();
    CmdLineObj.MarkerIm = markArg.getValue();
    CmdLineObj.MarkWSLine = lineArg.getValue();

    }
  catch (ArgException &e)  // catch any exceptions
    {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
}
////////////////////////////////////////////////////////
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
    TInput1 C = B - A;
    return static_cast< TOutput > (C.GetNorm());
  }
};
////////////////////////////////////////////////////////

template <class PixType, class LabPixType, int dim>
void doWatershed(const CmdLineType &CmdLineObj)
{
  typedef typename itk::VectorImage<PixType, dim> RawImType;
  typedef typename itk::Image<LabPixType, dim> LabImType;

  typedef DifPriority<typename RawImType::PixelType, 
		      double > DiffP;
  typedef typename itk::DisSimMorphologicalWatershedFromMarkersImageFilter<RawImType, 
									   LabImType,
									   DiffP> WSFiltType2;

  typename RawImType::Pointer input = readIm<RawImType>(CmdLineObj.InputIm);
  
  // Dissimilarity transform
  typename WSFiltType2::Pointer wsfilt = WSFiltType2::New();
  wsfilt->SetInput(input);
  wsfilt->SetMarkWatershedLine(CmdLineObj.MarkWSLine);
  wsfilt->SetMarkerImage(readIm<LabImType>(CmdLineObj.MarkerIm));
  wsfilt->SetFullyConnected(true);
  std::cout << "started dissimilarity watershed" << std::endl;
  typename LabImType::Pointer res = wsfilt->GetOutput();
  res->Update();
  res->DisconnectPipeline();
  //res->CopyInformation(raw);
  writeIm<LabImType>(res, CmdLineObj.OutputIm);


}
////////////////////////////////////////////////////////
int main(int argc, char * argv[])
{

  int dim1, dim2 = 0;
  CmdLineType CmdLineObj;
  ParseCmdLine(argc, argv, CmdLineObj);
//  itk::MultiThreader::SetGlobalMaximumNumberOfThreads(1);
  itk::ImageIOBase::IOComponentType ComponentType, MarkerComponentType;

  if (!readImageInfo(CmdLineObj.InputIm, &ComponentType, &dim1))
    {
    std::cerr << "Failed to open " << CmdLineObj.InputIm << std::endl;
    return(EXIT_FAILURE);
    }
  if (!readImageInfo(CmdLineObj.MarkerIm, &MarkerComponentType, &dim2))
    {
    std::cerr << "Failed to open " << CmdLineObj.MarkerIm << std::endl;
    return(EXIT_FAILURE);
    }

  if (dim1 != dim2)
    {
    std::cerr << "Image dimensions must match " << dim1 << " " << dim2 << std::endl;
    return(EXIT_FAILURE);

    }
  
// this will be a big, ugly switch statement to handle all the cases
// we want char, short and int markers, char, short, unsigned short,
// and float raw images
  switch (dim1)
    {
    case 2:
    {
#define WSDIM 2
#include "nasty_switch.h"
#undef WSDIM
    }
    break;
    case 3:
    {
#define WSDIM 3
#include "nasty_switch.h"
#undef WSDIM
    }
    break;
    default:
      std::cerr << "Unsupported dimension" << std::endl;
      break;
    }

  return EXIT_SUCCESS;
}
