
#ifndef __itkIFTWatershedFromMarkersImageFilter_h
#define __itkIFTWatershedFromMarkersImageFilter_h

#include "itkImageToImageFilter.h"


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
class IFTWSPriority
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
    return static_cast< TOutput >( B - A );
  }
};

}


template< class TInputImage, class TLabelImage >
class ITK_EXPORT IFTWatershedFromMarkersImageFilter:
    public IFTWatershedFromMarkersBaseImageFilter< TInputImage, TLabelImage, 
						   Functor::IFTPriority<
						     typename TInputImage::PixelType,
						     typename itk::NumericTraits<typename TInputImage::PixelType>::RealType PriorityType> >
{
public:
  /** Standard class typedefs. */
  typedef IFTWatershedFromMarkersImageFilter   Self;
  typedef IFTWatershedFromMarkersBaseImageFilter< 
  TInputImage, TLabelImage, 
    Functor::IFTPriority<
  typename TInputImage::PixelType,
    typename itk::NumericTraits<typename TInputImage::PixelType>::RealType PriorityType> > Superclass;

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

  typedef typename TPriorityFunction PriorityFunctorType;


  /** ImageDimension constants */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(IFTWatershedFromMarkersBaseImageFilter,
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


  /**
   * Set/Get functors controlling the priority. This controls which
   * form of watershed you get
   */
  PriorityFunctorType & GetFunctor() { return m_PriorityFunctor; }

  void SetFunctor(const PriorityFunctorType & functor)
  {
    if ( m_Functor != functor )
      {
      m_PriorityFunctor = functor;
      this->Modified();
      }
  }

protected:
  IFTWatershedFromMarkersBaseImageFilter();
  ~IFTWatershedFromMarkersBaseImageFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const;

  /** IFTWatershedFromMarkersBaseImageFilter needs to request the
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
  IFTWatershedFromMarkersBaseImageFilter(const Self &);
  void operator=(const Self &); //purposely not implemented

  bool m_FullyConnected;

  bool m_MarkWatershedLine;


#ifdef QUEUEA
  // typedefs for the double queue structure
  typedef typename itk::NumericTraits<InputImagePixelType>::RealType PriorityType;
  typedef long IterationType;

  // priority has two elements, to preserve fifo ordering on plateaus
  typedef class CombPriorityType {
  public:
    PriorityType P;
    IterationType time;

  } CombPriorityType;

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

  typedef IFTQueueA<CombPriorityType, IndexType, ComparePriority, 
		    typename IndexType::LexicographicCompare> DoubleQueueType;
#else
    // alternative version that doesn't use two elements in the
    // priority class, but needs to do a search within the list at the
    // specific priority to find the voxel.
#endif


  PriorityFunctorType m_PriorityFunctor;
}; // end of class
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkIFTWatershedFromMarkersBaseImageFilter.hxx"
#endif

#endif
