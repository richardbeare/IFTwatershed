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
#ifndef __itkDisSimMorphologicalWatershedFromMarkersImageFilter_h
#define __itkDisSimMorphologicalWatershedFromMarkersImageFilter_h

#include "itkImageToImageFilter.h"

namespace itk
{
/** \class DisSimMorphologicalWatershedFromMarkersImageFilter
 * \brief Morphological watershed transform from markers, with the
 * option to compute a higher resolution gradient internally. This
 * also gives the option on using it for colour images.
 *
 * This class is a minor variation on the standard ITK
 * MorphologicalWatershedFromMarkersImageFilter, and was developed for
 * comparison with the image forest transform algorithm.
 *
 * \author Richard Beare. Department of Medicine, Monash University,
 * Melbourne, Australia  and Developmental Imaging, Murdoch Childrens
 * Research Institute, Royal Childrens Hospital, Melbourne, Australia.
 *
 * \sa WatershedImageFilter, DisSimMorphologicalWatershedImageFilter
 * \ingroup ImageEnhancement  MathematicalMorphologyImageFilters
 * \ingroup ITKReview
 */
template< class TInputImage, class TLabelImage, class TPriorityFunction >
class ITK_EXPORT DisSimMorphologicalWatershedFromMarkersImageFilter:
  public ImageToImageFilter< TInputImage, TLabelImage >
{
public:
  /** Standard class typedefs. */
  typedef DisSimMorphologicalWatershedFromMarkersImageFilter   Self;
  typedef ImageToImageFilter< TInputImage, TLabelImage > Superclass;
  typedef SmartPointer< Self >                           Pointer;
  typedef SmartPointer< const Self >                     ConstPointer;

  /** Some convenient typedefs. */
  typedef TInputImage                           InputImageType;
  typedef TLabelImage                           LabelImageType;
  typedef typename InputImageType::Pointer      InputImagePointer;
  typedef typename InputImageType::ConstPointer InputImageConstPointer;
  typedef typename InputImageType::RegionType   InputImageRegionType;
  typedef typename InputImageType::PixelType    InputImagePixelType;
  typedef typename LabelImageType::Pointer      LabelImagePointer;
  typedef typename LabelImageType::ConstPointer LabelImageConstPointer;
  typedef typename LabelImageType::RegionType   LabelImageRegionType;
  typedef typename LabelImageType::PixelType    LabelImagePixelType;

  typedef typename LabelImageType::IndexType IndexType;

  typedef TPriorityFunction PriorityFunctorType;
  // this is probably overkill
  typedef typename itk::NumericTraits<InputImagePixelType>::RealType PriorityType;


  /** ImageDimension constants */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(DisSimMorphologicalWatershedFromMarkersImageFilter,
               ImageToImageFilter);

  /** Set the marker image */
  void SetMarkerImage(const TLabelImage *input)
  {
    // Process object is not const-correct so the const casting is required.
    this->SetNthInput( 1, const_cast< TLabelImage * >( input ) );
  }

  /** Get the marker image */
  const LabelImageType * GetMarkerImage() const
  {
    return static_cast< LabelImageType * >(
             const_cast< DataObject * >( this->ProcessObject::GetInput(1) ) );
  }

  /** Set the input image */
  void SetInput1(const TInputImage *input)
  {
    this->SetInput(input);
  }

  /** Set the marker image */
  void SetInput2(const TLabelImage *input)
  {
    this->SetMarkerImage(input);
  }

  /**
   * Set/Get whether the connected components are defined strictly by
   * face connectivity or by face+edge+vertex connectivity.  Default is
   * FullyConnectedOff.  For objects that are 1 pixel wide, use
   * FullyConnectedOn.
   */
  itkSetMacro(FullyConnected, bool);
  itkGetConstReferenceMacro(FullyConnected, bool);
  itkBooleanMacro(FullyConnected);

  /**
   * Set/Get whether the watershed pixel must be marked or not. Default
   * is true. Set it to false do not only avoid writing watershed pixels,
   * it also decrease algorithm complexity.
   */
  itkSetMacro(MarkWatershedLine, bool);
  itkGetConstReferenceMacro(MarkWatershedLine, bool);
  itkBooleanMacro(MarkWatershedLine);
protected:
  DisSimMorphologicalWatershedFromMarkersImageFilter();
  ~DisSimMorphologicalWatershedFromMarkersImageFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const;

  /** DisSimMorphologicalWatershedFromMarkersImageFilter needs to request the
   * entire input images.
   */
  void GenerateInputRequestedRegion();

  /** This filter will enlarge the output requested region to produce
   * all of the output.
   * \sa ProcessObject::EnlargeOutputRequestedRegion() */
  void EnlargeOutputRequestedRegion( DataObject *itkNotUsed(output) );

  /** The filter is single threaded. */
  void GenerateData();

private:
  //purposely not implemented
  DisSimMorphologicalWatershedFromMarkersImageFilter(const Self &);
  void operator=(const Self &); //purposely not implemented

  bool m_FullyConnected;

  bool m_MarkWatershedLine;
  PriorityFunctorType m_PriorityFunctor;
}; // end of class
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkDisSimMorphologicalWatershedFromMarkersImageFilter.hxx"
#endif

#endif
