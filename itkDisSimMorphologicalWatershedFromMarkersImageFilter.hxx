/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __itkDisSimMorphologicalWatershedFromMarkersImageFilter_hxx
#define __itkDisSimMorphologicalWatershedFromMarkersImageFilter_hxx

#include <algorithm>
#include <queue>
#include <list>
#include "itkDisSimMorphologicalWatershedFromMarkersImageFilter.h"
#include "itkProgressReporter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkConstShapedNeighborhoodIterator.h"
#include "itkConstantBoundaryCondition.h"
#include "itkSize.h"
#include "itkConnectedComponentAlgorithm.h"

namespace itk
{
template< class TInputImage, class TLabelImage, class TPriorityFunction >
DisSimMorphologicalWatershedFromMarkersImageFilter< TInputImage, TLabelImage, TPriorityFunction >
::DisSimMorphologicalWatershedFromMarkersImageFilter()
{
  this->SetNumberOfRequiredInputs(2);
  m_FullyConnected = false;
  m_MarkWatershedLine = true;
}

template< class TInputImage, class TLabelImage, class TPriorityFunction >
void
DisSimMorphologicalWatershedFromMarkersImageFilter< TInputImage, TLabelImage, TPriorityFunction >
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

template< class TInputImage, class TLabelImage, class TPriorityFunction >
void
DisSimMorphologicalWatershedFromMarkersImageFilter< TInputImage, TLabelImage, TPriorityFunction >
::EnlargeOutputRequestedRegion(DataObject *)
{
  this->GetOutput()->SetRequestedRegion(
    this->GetOutput()->GetLargestPossibleRegion() );
}

template< class TInputImage, class TLabelImage, class TPriorityFunction >
void
DisSimMorphologicalWatershedFromMarkersImageFilter< TInputImage, TLabelImage, TPriorityFunction >
::GenerateData()
{
  // there is 2 possible cases: with or without watershed lines.
  // the algorithm with watershed lines is from Meyer
  // the algorithm without watershed lines is from beucher
  // The 2 algorithms are very similar and so are integrated in the same filter.

  //---------------------------------------------------------------------------
  // declare the vars common to the 2 algorithms: constants, iterators,
  // hierarchical queue, progress reporter, and status image
  // also allocate output images and verify preconditions
  //---------------------------------------------------------------------------

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
  typedef std::queue< IndexType >                    QueueType;
  typedef std::map< PriorityType, QueueType > MapType;
  MapType fah;

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

  //---------------------------------------------------------------------------
  // Meyer's algorithm
  //---------------------------------------------------------------------------
    
    // first stage:
    //  - set markers pixels to already processed status
    //  - copy markers pixels to output image
    //  - init FAH with indexes of background pixels with marker pixel(s) in
    //    their neighborhood

    ConstantBoundaryCondition< LabelImageType > lcbc2;
    // outside pixel are watershed so they won't be use to find real watershed
    // pixels
    lcbc2.SetConstant(wsLabel);
    outputIt.OverrideBoundaryCondition(&lcbc2);

    // create a temporary image to store the state of each pixel (processed or
    // not)
    typedef Image< bool, ImageDimension > StatusImageType;
    typename StatusImageType::Pointer statusImage = StatusImageType::New();
    statusImage->SetRegions( markerImage->GetLargestPossibleRegion() );
    statusImage->Allocate();

    // iterator for the status image
    typedef ShapedNeighborhoodIterator< StatusImageType > StatusIteratorType;
    typename StatusIteratorType::Iterator nsIt;
    StatusIteratorType
                                                 statusIt( radius, statusImage, outputImage->GetRequestedRegion() );
    ConstantBoundaryCondition< StatusImageType > bcbc;
    bcbc.SetConstant(true);    // outside pixel are already processed
    statusIt.OverrideBoundaryCondition(&bcbc);
    setConnectivity(&statusIt, m_FullyConnected);

    // the status image must be initialized before the first stage. In the
    // first stage, the set to true are the neighbors of the marker (and the
    // marker) so it's difficult (impossible ?) to init the status image at
    // the same time
    // the overhead should be small
    //
    // In the dissimilarity framework the status image is used to
    // track whether a voxel is a watershed line.
    // Otherwise we use the output image to track whether a voxel is
    // already labelled. This allows voxels to be inserted in the
    // queue multiple times, but only the highest priority entry
    // will be used.
    
    statusImage->FillBuffer(false);

    for ( markerIt.GoToBegin(), statusIt.GoToBegin(), outputIt.GoToBegin(), inputIt.GoToBegin();
          !markerIt.IsAtEnd();
          ++markerIt, ++outputIt )
      {
      LabelImagePixelType markerPixel = markerIt.GetCenterPixel();
      if ( markerPixel != bgLabel )
        {
        IndexType idx = markerIt.GetIndex();

        // move the iterators to the right place
        OffsetType shift = idx - statusIt.GetIndex();
        statusIt += shift;
        inputIt += shift;

        // this pixel belongs to a marker
        // mark it as already processed
        statusIt.SetCenterPixel(true);
        // copy it to the output image
        outputIt.SetCenterPixel(markerPixel);
        // and increase progress because this pixel will not be used in the
        // flooding stage.
        progress.CompletedPixel();

        // search the background pixels in the neighborhood
        for ( nmIt = markerIt.Begin(), nsIt = statusIt.Begin(), niIt = inputIt.Begin();
              nmIt != markerIt.End();
              nmIt++, nsIt++, niIt++ )
          {
          if ( !nsIt.Get() && nmIt.Get() == bgLabel )
            {
            // this neighbor is a background pixel and is not already
            // processed; add its index to fah
	    // Priority depends on neighbour combinations, so needs to support
            // pixels being put on more than once
	    PriorityType priority = m_PriorityFunctor(inputIt.GetCenterPixel(), niIt.Get());
            fah[priority].push( markerIt.GetIndex()
                                  + nmIt.GetNeighborhoodOffset() );
            // mark it as already in the fah to avoid adding it several times
            // nsIt.Set(true);
            }
          }
        }
      else
        {
        // Some pixels may be never processed so, by default, non marked pixels
        // must be marked as watershed
        outputIt.SetCenterPixel(wsLabel);
        }
      // one more pixel done in the init stage
      progress.CompletedPixel();
      }
    // flooding
    // init all the iterators
    outputIt.GoToBegin();
    statusIt.GoToBegin();
    inputIt.GoToBegin();

    // and start flooding
    while ( !fah.empty() )
      {
      // store the current vars
      PriorityType currentValue = fah.begin()->first;
      QueueType    currentQueue = fah.begin()->second;
      // and remove them from the fah
      fah.erase( fah.begin() );

      while ( !currentQueue.empty() )
        {
        IndexType idx = currentQueue.front();
        currentQueue.pop();

        // move the iterators to the right place
        OffsetType shift = idx - outputIt.GetIndex();
        outputIt += shift;
        statusIt += shift;
        inputIt += shift;

	// skip if we've already labelled it.
	if (outputIt.GetCenterPixel() != wsLabel) continue;
        // iterate over the neighbors. If there is only one marker value, give
        // that value to the pixel, else keep it as is (watershed line)
        LabelImagePixelType marker = wsLabel;
        bool                collision = false;
        for ( noIt = outputIt.Begin(); noIt != outputIt.End(); noIt++ )
          {
          LabelImagePixelType o = noIt.Get();
          if ( o != wsLabel )
            {
            if ( marker != wsLabel && o != marker )
              {
              collision = true;
              break;
              }
            else
                  { marker = o; }
            }
          }
        if ( !collision )
          {
          // set the marker value
          outputIt.SetCenterPixel(marker);
          // and propagate to the neighbors
          for ( niIt = inputIt.Begin(), nsIt = statusIt.Begin(), noIt = outputIt.Begin();
                niIt != inputIt.End();
                niIt++, nsIt++, noIt++ )
            {
	    // can't be a watershed line or already labelled
            if ( !nsIt.Get() & (noIt.Get() == wsLabel)  ) 
              {
              // the pixel is not yet processed. add it to the fah
              //InputImagePixelType GrayVal = niIt.Get();
	      PriorityType priority = m_PriorityFunctor(inputIt.GetCenterPixel(), niIt.Get());

              if ( priority <= 0 )
                {
                currentQueue.push( inputIt.GetIndex()
                                   + niIt.GetNeighborhoodOffset() );
                }
              else
                {
                fah[priority].push( inputIt.GetIndex()
                                   + niIt.GetNeighborhoodOffset() );
                }
              // mark it as already in the fah
              // nsIt.Set(true);
              }
            }
          }
	else
	  {
	   // mark the status image - watershed lines can't change
	   statusIt.SetCenterPixel(true);
	  }
        // one more pixel in the flooding stage
        progress.CompletedPixel();
        }
      }
    

  // Beucher's algorithm doesn't translate easily to dissimilarity style - forget it and
  // hack the Meyer version
    if ( !m_MarkWatershedLine )
      {
      // iterate over the output image and fill in the watershed line
      for ( outputIt.GoToBegin(), inputIt.GoToBegin();
	    !outputIt.IsAtEnd(); ++outputIt )
	{
	LabelImagePixelType markerPixel = outputIt.GetCenterPixel();
	if ( markerPixel == bgLabel )
	  {
	  // watershed line - find the most similar neighbour (lowest difference)
	  IndexType idx = outputIt.GetIndex();
	  // move the iterators to the right place
	  OffsetType shift = idx - inputIt.GetIndex();
	  inputIt += shift;
	  PriorityType bestpriority = itk::NumericTraits<PriorityType>::max();
	  LabelImagePixelType M = wsLabel;
	  for ( nmIt = outputIt.Begin(), niIt = inputIt.Begin();
		nmIt != outputIt.End();
		nmIt++, niIt++ )
	    {
	    PriorityType priority = m_PriorityFunctor(inputIt.GetCenterPixel(), niIt.Get());
	    if (priority < bestpriority)
	      {
	      bestpriority = priority;
	      M = nmIt.Get();
	      }
	    }
	  outputIt.SetCenterPixel(M);
	  }
	}
      }
    
}

template< class TInputImage, class TLabelImage, class TPriorityFunction >
void
DisSimMorphologicalWatershedFromMarkersImageFilter< TInputImage, TLabelImage, TPriorityFunction >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "FullyConnected: "  << m_FullyConnected << std::endl;
  os << indent << "MarkWatershedLine: "  << m_MarkWatershedLine << std::endl;
}
} // end namespace itk
#endif
