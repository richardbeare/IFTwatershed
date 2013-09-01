
#ifndef __itkIFTWatershedFromMarkersImageFilter_h
#define __itkIFTWatershedFromMarkersImageFilter_h

#include "itkIFTWatershedFromMarkersBaseImageFilter.h"


namespace itk
{
/** \class IFTWatershedFromMarkersImageFilter
 * \brief IFT watershed transform from markers
 *
 * This is an instantiation of the base image filter with the IFT
 * functor. It can be changed to get a conventional watershed.
 *
 * "The ordered queue and the optimality of the watershed
 * approaches." Roberto Lotufo and Alexandre Falcao, In Mathematical
 * Morphology and its Applications to Image and Signal Processing,
 * 2000
 *
 * \author Richard Beare. Department of Medicine, Monash University,
 * Melbourne, Australia and Murdoch Childrens Research Institute,
 * Royal Childrens Hospital, Melbourne, Australia.
 *
 * \sa WatershedImageFilter, IFTWatershedFromMarkersBaseImageFilter, MorphologicalWatershedFromMarkersImageFilter
 * \ingroup ImageEnhancement  MathematicalMorphologyImageFilters
 */

/* functors to define the IFT cost?? */
/* One for similarity watershed, one for traditional watershed ? */

namespace Functor
{
/* functors to define the IFT cost?? */
/* One for similarity watershed, one for traditional watershed ? */

template< class TInput1, class TOutput = TInput1 >
class IFTWSPriority
{
public:
  typedef typename NumericTraits< TInput1 >::AccumulateType AccumulatorType;
  IFTWSPriority() {}
  ~IFTWSPriority() {}
  bool operator!=(const IFTWSPriority &) const
  {
    return false;
  }

  bool operator==(const IFTWSPriority & other) const
  {
    return !( *this != other );
  }

  // A is the centre pixel, B the neighbour
  inline TOutput operator()(const TInput1 & A, const TInput1 & B) const
  {
    return static_cast< TOutput >( B );
  }
};

template< class TInput1, class TOutput = TInput1 >
class IFTPriority
{
public:
  typedef typename NumericTraits< TInput1 >::AccumulateType AccumulatorType;
  IFTPriority() {}
  ~IFTPriority() {}
  bool operator!=(const IFTPriority &) const
  {
    return false;
  }

  bool operator==(const IFTPriority & other) const
  {
    return !( *this != other );
  }

  // A is the centre pixel, B the neighbour
  inline TOutput operator()(const TInput1 & A, const TInput1 & B) const
  {
    return static_cast< TOutput >(vcl_abs( B - A ));
  }
};

}


template< class TInputImage, class TLabelImage >
class ITK_EXPORT IFTWatershedFromMarkersImageFilter:
    public IFTWatershedFromMarkersBaseImageFilter< TInputImage, TLabelImage, 
						   typename Functor::IFTPriority<
						     typename TInputImage::PixelType,
						     typename itk::NumericTraits<typename TInputImage::PixelType>::RealType > >
{
public:
  /** Standard class typedefs. */
  typedef IFTWatershedFromMarkersImageFilter   Self;
  typedef IFTWatershedFromMarkersBaseImageFilter< 
  TInputImage, TLabelImage, 
    Functor::IFTPriority<
  typename TInputImage::PixelType,
    typename itk::NumericTraits<typename TInputImage::PixelType>::RealType > > Superclass;

  typedef SmartPointer< Self >                           Pointer;
  typedef SmartPointer< const Self >                     ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(IFTWatershedFromMarkersImageFilter,
               IFTWatershedFromMarkersBaseImageFilter);

protected:
  IFTWatershedFromMarkersImageFilter(){}
  ~IFTWatershedFromMarkersImageFilter() {}

private:
  //purposely not implemented
  IFTWatershedFromMarkersImageFilter(const Self &);
  void operator=(const Self &); //purposely not implemented

}; // end of class
} // end namespace itk

#endif
