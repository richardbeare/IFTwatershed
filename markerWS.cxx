#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>
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
  float scale;
  bool morphGrad, MarkWSLine, dissim, FullyConnected;
} CmdLineType;

void ParseCmdLine(int argc, char* argv[],
                  CmdLineType &CmdLineObj
                  )
{
  using namespace TCLAP;
  try
    {
    // Define the command line object.
    CmdLine cmd("scaleWS ", ' ', "0.9");

    ValueArg<std::string> inArg("i","input","input image",true,"result","string");
    cmd.add( inArg );

    ValueArg<std::string> markArg("m","marker","marker image",true,"result","string");
    cmd.add( markArg );

    ValueArg<std::string> outArg("o","output","output image", true,"","string");
    cmd.add( outArg );

    ValueArg<float> scaleArg("s","scale","scale of smoothing of gradient, in (mm) if not also morphgrad", false, 1,"float");
    cmd.add(scaleArg);

    // should change this to switch arg
    SwitchArg morphArg("g","morphgrad","Use a morphological gradient, scale becomes pixels", false);
    cmd.add(morphArg);

    SwitchArg lineArg("l","markline","Mark the watershed line", false);
    cmd.add(lineArg);

    SwitchArg conArg("","FullyConnected","connectivity", false);
    cmd.add(conArg);

    SwitchArg disArg("","dissimilarity","use a dissimilarity watershed (internal gradient calculation - all gradient stuff is ignored", false);
    cmd.add(disArg);

    // Parse the args.
    cmd.parse( argc, argv );

    CmdLineObj.InputIm = inArg.getValue();
    CmdLineObj.OutputIm = outArg.getValue();
    CmdLineObj.MarkerIm = markArg.getValue();
    CmdLineObj.scale = scaleArg.getValue();
    CmdLineObj.morphGrad = morphArg.getValue();
    CmdLineObj.MarkWSLine = lineArg.getValue();
    CmdLineObj.dissim = disArg.getValue();
    CmdLineObj.FullyConnected = conArg.getValue();

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
    return static_cast< TOutput > (vcl_abs( B - A ));
  }
};
////////////////////////////////////////////////////////

template <class PixType, class LabPixType, int dim>
void doWatershed(const CmdLineType &CmdLineObj)
{
  typedef typename itk::Image<PixType, dim> RawImType;
  typedef typename itk::Image<LabPixType, dim> LabImType;

  // gradients computed using the raw pixel type
  typedef typename itk::GradientMagnitudeRecursiveGaussianImageFilter<RawImType,RawImType> GradFiltType;
  typedef typename itk::FlatStructuringElement<dim> KernType;
  typedef typename itk::MorphologicalGradientImageFilter<RawImType,RawImType, KernType> MorphGradFiltType;

  typedef typename itk::MorphologicalWatershedFromMarkersImageFilter<RawImType, LabImType> WSFiltType;
  typedef DifPriority<PixType, 
		      typename itk::NumericTraits<PixType>::FloatType > DiffP;
  typedef typename itk::DisSimMorphologicalWatershedFromMarkersImageFilter<RawImType, 
									   LabImType,
									   DiffP> WSFiltType2;

  typename RawImType::Pointer input = readIm<RawImType>(CmdLineObj.InputIm);
  typename RawImType::Pointer grad;
  
  if (CmdLineObj.dissim) 
    {
    // Dissimilarity transform
    typename WSFiltType2::Pointer wsfilt = WSFiltType2::New();
    wsfilt->SetInput(input);
    wsfilt->SetMarkWatershedLine(CmdLineObj.MarkWSLine);
    wsfilt->SetFullyConnected(CmdLineObj.FullyConnected);
    // wsfilt->SetMarkerImage(orienter->GetOutput());
    wsfilt->SetMarkerImage(readIm<LabImType>(CmdLineObj.MarkerIm));
    std::cout << "started dissimilarity watershed" << std::endl;
    typename LabImType::Pointer res = wsfilt->GetOutput();
    res->Update();
    res->DisconnectPipeline();
    //res->CopyInformation(raw);
    writeIm<LabImType>(res, CmdLineObj.OutputIm);
    }
  else 
    {
    if (CmdLineObj.scale != 0.0)
      {
      if (CmdLineObj.morphGrad)
	{
#ifndef USEPARA
	// using a morphological gradient - something wrong with this at present
	typename MorphGradFiltType::Pointer morphgrad = MorphGradFiltType::New();
	typename KernType::RadiusType radius;
	radius.Fill(int(CmdLineObj.scale));
	KernType kern = KernType::Box(radius);
	
	morphgrad->SetInput(input);
	morphgrad->SetKernel(kern);
	grad = morphgrad->GetOutput();
	grad->Update();
	grad->DisconnectPipeline();
#else
	// use parabolic instead
	typedef typename itk::ParabolicErodeImageFilter<RawImType> EPType;
	typedef typename itk::ParabolicDilateImageFilter<RawImType> DPType;
	typedef typename itk::SubtractImageFilter<RawImType, RawImType, RawImType> SType;
	
	typename DPType::Pointer dilate = DPType::New();
	typename EPType::Pointer erode = EPType::New();
	typename SType::Pointer sub = SType::New();
	
	dilate->SetInput(input);
	erode->SetInput(dilate->GetOutput());
	sub->SetInput1(dilate->GetOutput());
	sub->SetInput2(erode->GetOutput());
	
	dilate->SetUseImageSpacing(true);
	erode->SetUseImageSpacing(true);
	
	dilate->SetScale(CmdLineObj.scale);
	erode->SetScale(CmdLineObj.scale);
	grad = sub->GetOutput();
	grad->Update();
	grad->DisconnectPipeline();

#endif 
	}
      else
	{
	typename GradFiltType::Pointer gradfilt = GradFiltType::New();
	gradfilt->SetInput(input);
	gradfilt->SetSigma(CmdLineObj.scale);
	grad = gradfilt->GetOutput();
	grad->Update();
	grad->DisconnectPipeline();
	
	
	}
      }
    else
      {
      std::cout << "No gradient being used" << std::endl;
      grad = input;
      }

    // orientation related parts break the 2D case
    typedef typename itk::OrientImageFilter<LabImType, LabImType> OrientType;
    // typename OrientType::Pointer orienter = OrientType::New();
    // itk::SpatialOrientationAdapter orientAd;

    // orienter->SetInput(readIm<LabImType>(CmdLineObj.MarkerIm));
    // orienter->UseImageDirectionOn();
    // orienter->SetDesiredCoordinateOrientation(orientAd.FromDirectionCosines(grad->GetDirection()));
    typename WSFiltType::Pointer wsfilt = WSFiltType::New();
    wsfilt->SetInput(grad);
    wsfilt->SetMarkWatershedLine(CmdLineObj.MarkWSLine);
    wsfilt->SetFullyConnected(CmdLineObj.FullyConnected);
    // wsfilt->SetMarkerImage(orienter->GetOutput());
    wsfilt->SetMarkerImage(readIm<LabImType>(CmdLineObj.MarkerIm));
    std::cout << "started watershed" << std::endl;
    typename LabImType::Pointer res = wsfilt->GetOutput();
    res->Update();
    res->DisconnectPipeline();
    //res->CopyInformation(raw);
    writeIm<LabImType>(res, CmdLineObj.OutputIm);
    writeIm<RawImType>(grad, "grad.nii.gz");
    }

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
