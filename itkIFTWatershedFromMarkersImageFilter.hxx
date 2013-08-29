#ifndef __itkIFTWatershedFromMarkersImageFilter_hxx
#define __itkIFTWatershedFromMarkersImageFilter_hxx

#include "itkIFTWatershedFromMarkersImageFilter.h"
#include "itkProgressReporter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkConstShapedNeighborhoodIterator.h"
#include "itkConstantBoundaryCondition.h"
#include "itkSize.h"
#include "itkConnectedComponentAlgorithm.h"


namespace itk
{
template< class TInputImage, class TLabelImage >
IFTWatershedFromMarkersImageFilter< TInputImage, TLabelImage >
::IFTWatershedFromMarkersImageFilter()
{
  this->SetNumberOfRequiredInputs(2);
  m_FullyConnected = false;
  m_MarkWatershedLine = true;
}

template< class TInputImage, class TLabelImage >
void
IFTWatershedFromMarkersImageFilter< TInputImage, TLabelImage >
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the inputs
  LabelImagePointer markerPtr =
    const_cast< LabelImageType * >( this->GetMarkerImage() );

  InputImagePointer inputPtr =
    const_cast< InputImageType * >( this->GetInput() );

  if ( !markerPtr || !inputPtr )
        { return; }

  // We need to
  // configure the inputs such that all the data is available.
  //
  markerPtr->SetRequestedRegion( markerPtr->GetLargestPossibleRegion() );
  inputPtr->SetRequestedRegion( inputPtr->GetLargestPossibleRegion() );
}

template< class TInputImage, class TLabelImage >
void
IFTWatershedFromMarkersImageFilter< TInputImage, TLabelImage >
::EnlargeOutputRequestedRegion(DataObject *)
{
  this->GetOutput()->SetRequestedRegion(
    this->GetOutput()->GetLargestPossibleRegion() );
}

template< class TInputImage, class TLabelImage >
void
IFTWatershedFromMarkersImageFilter< TInputImage, TLabelImage >
::GenerateData()
{

  // the label used to find background in the marker image
  static const LabelImagePixelType bgLabel =
    NumericTraits< LabelImagePixelType >::Zero;
  // the label used to mark the watershed line in the output image
  static const LabelImagePixelType wsLabel =
    NumericTraits< LabelImagePixelType >::Zero;

  this->AllocateOutputs();

  LabelImageConstPointer markerImage = this->GetMarkerImage();
  InputImageConstPointer inputImage = this->GetInput();
  LabelImagePointer      outputImage = this->GetOutput();

  // Set up the progress reporter
  // we can't found the exact number of pixel to process in the 2nd pass, so we
  // use the maximum number possible.
  ProgressReporter
  progress(this, 0, markerImage->GetRequestedRegion().GetNumberOfPixels() * 2);

  // mask and marker must have the same size
  if ( markerImage->GetRequestedRegion().GetSize() != inputImage->GetRequestedRegion().GetSize() )
    {
    itkExceptionMacro(<< "Marker and input must have the same size.");
    }

  // FAH (in french: File d'Attente Hierarchique)
  DoubleQueueType fah;

  // the radius which will be used for all the shaped iterators
  Size< ImageDimension > radius;
  radius.Fill(1);

  // iterator for the marker image
  typedef ConstShapedNeighborhoodIterator< LabelImageType > MarkerIteratorType;
  typename MarkerIteratorType::ConstIterator nmIt;
  MarkerIteratorType
  markerIt( radius, markerImage, markerImage->GetRequestedRegion() );
  // add a boundary constant to avoid adding pixels on the border in the fah
  ConstantBoundaryCondition< LabelImageType > lcbc;
  lcbc.SetConstant( NumericTraits< LabelImagePixelType >::max() );
  markerIt.OverrideBoundaryCondition(&lcbc);
  setConnectivity(&markerIt, m_FullyConnected);

  // iterator for the input image
  typedef ConstShapedNeighborhoodIterator< InputImageType > InputIteratorType;
  InputIteratorType
  inputIt( radius, inputImage, inputImage->GetRequestedRegion() );
  typename InputIteratorType::ConstIterator niIt;
  setConnectivity(&inputIt, m_FullyConnected);

  // iterator for the output image
  typedef ShapedNeighborhoodIterator< LabelImageType > OutputIteratorType;
  typedef typename OutputIteratorType::OffsetType      OffsetType;
  typename OutputIteratorType::Iterator noIt;
  OutputIteratorType
  outputIt( radius, outputImage, outputImage->GetRequestedRegion() );
  setConnectivity(&outputIt, m_FullyConnected);

  // setting up the queue by looking for borders of the markers
  ConstantBoundaryCondition< LabelImageType > lcbc2;
  // outside pixel are watershed so they won't be use to find real watershed
  // pixels
  lcbc2.SetConstant(wsLabel);
  outputIt.OverrideBoundaryCondition(&lcbc2);

  // create a temporary image to store the state of each pixel (processed or
  // not) - this is the "flag" image in the paper
  typedef Image< bool, ImageDimension > StatusImageType;
  typename StatusImageType::Pointer flagImage = StatusImageType::New();
  flagImage->SetRegions( markerImage->GetLargestPossibleRegion() );
  flagImage->Allocate();

  // a temporary cost image
  typedef Image< PriorityType, ImageDimension > PriorityImageType;
  typename PriorityImageType::Pointer costImage = PriorityImageType::New();
  costImage->SetRegions( markerImage->GetLargestPossibleRegion() );
  costImage->Allocate();

  // iterator for the flag image
  typedef ShapedNeighborhoodIterator< StatusImageType > StatusIteratorType;
  typename StatusIteratorType::Iterator flIt;
  StatusIteratorType
    flagIt( radius, flagImage, outputImage->GetRequestedRegion() );
  ConstantBoundaryCondition< StatusImageType > bcbc;
  bcbc.SetConstant(true);    // outside pixel are already processed
  flagIt.OverrideBoundaryCondition(&bcbc);
  setConnectivity(&flagIt, m_FullyConnected);
  flagImage->FillBuffer(false);

  // iterator for the cost image
  typedef ShapedNeighborhoodIterator< PriorityImageType > CostIteratorType;
  typename CostIteratorType::Iterator ncIt;
  CostIteratorType
    costIt( radius, costImage, outputImage->GetRequestedRegion() );
  ConstantBoundaryCondition< PriorityImageType > costbc;
  costbc.SetConstant(true);    // outside pixel are already processed
  costIt.OverrideBoundaryCondition(&costbc);
  setConnectivity(&costIt, m_FullyConnected);
  costImage->FillBuffer(itk::NumericTraits<PriorityType>::max());

#ifdef QUEUEA
  IterationType GlobalTime = 0;
#endif

  for ( markerIt.GoToBegin(), outputIt.GoToBegin(), inputIt.GoToBegin();
	!markerIt.IsAtEnd();
	++markerIt, ++outputIt )
    {
    LabelImagePixelType markerPixel = markerIt.GetCenterPixel();
    if ( markerPixel != bgLabel )
      {
      IndexType  idx = markerIt.GetIndex();
      OffsetType shift = idx - flagIt.GetIndex();
      flagIt += shift;
      costIt += shift;
      // this pixels belongs to a marker
      // copy it to the output image
      outputIt.SetCenterPixel(markerPixel);
      costIt.SetCenterPixel(0);
      // search if it has background pixel in its neighborhood
      bool haveBgNeighbor = false;
      for ( nmIt = markerIt.Begin(); nmIt != markerIt.End(); nmIt++ )
	{
	if ( nmIt.Get() == bgLabel )
	  {
	  haveBgNeighbor = true;
	  break;
	  }
	}
      if ( haveBgNeighbor )
	{
	// there is a background pixel in the neighborhood; add to fah
#ifdef QUEUEA
	CombPriorityType P;
	P.time=GlobalTime;
	++GlobalTime;
	P.P = 0;
	fah.insert(markerIt.GetIndex(), P);
#else

#endif
	}
      else
	{
	// increase progress because this pixel will not be used in the
	// flooding stage.
	// Need to mark it in the glag image as done
	flagIt.SetCenterPixel(true);
	progress.CompletedPixel();
	}
      }
    else
      {
      outputIt.SetCenterPixel(wsLabel);
      }
    progress.CompletedPixel();
    }
  // end of init stage
  outputIt.GoToBegin();
  flagIt.GoToBegin();
  inputIt.GoToBegin();
  costIt.GoToBegin();
  // and start flooding
  while ( !fah.empty() )
    {
#ifdef QUEUEA
    CombPriorityType CP = fah.front_key();
    IndexType idx = fah.front_value();
    fah.pop();
#else

#endif
    OffsetType shift = idx - outputIt.GetIndex();
    outputIt += shift;
    flagIt += shift;
    inputIt += shift;
    costIt += shift;

    flagIt.SetCenterPixel(true);
    // for each p neighbour of idx and flag[p]==false
    PriorityType CentreCost = costIt.GetCenterPixel();
    InputImagePixelType CentrePix = inputIt.GetCenterPixel();
    LabelImagePixelType CentreLab = outputIt.GetCenterPixel();
    // may be a way of optimizing these neighborhood iterators
    for (flIt = flagIt.Begin(), ncIt = costIt.Begin(), niIt = inputIt.Begin(), noIt =outputIt.Begin();
	 flIt != flagIt.End(); 
	 flIt++, ncIt++, niIt++, noIt++)
      {
      if (!flIt.Get())
	{
	PriorityType NeighCost = ncIt.Get();
	InputImagePixelType NeighVal = niIt.Get();
	// the function defining StepCost needs to be made general
	PriorityType StepCost = NeighVal - CentrePix;
	PriorityType NewCost = std::max(CentreCost, StepCost);
	if (NewCost < NeighCost)
	  {
	  ncIt.Set(NewCost);
	  noIt.Set(CentreLab);
#ifdef QUEUEA	  
	  CombPriorityType NP;
	  NP.P = NewCost;
	  NP.time = GlobalTime;
	  ++GlobalTime;
	  fah.insert(flagIt.GetIndex() + flIt.GetNeighborhoodOffset(), NP);
#else

#endif
	  }
	}
      }

    }
}

template< class TInputImage, class TLabelImage >
void
IFTWatershedFromMarkersImageFilter< TInputImage, TLabelImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "FullyConnected: "  << m_FullyConnected << std::endl;
  os << indent << "MarkWatershedLine: "  << m_MarkWatershedLine << std::endl;
}
} // end namespace itk
#endif
