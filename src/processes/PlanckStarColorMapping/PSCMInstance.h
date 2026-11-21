#ifndef __PSCMInstance_h
#define __PSCMInstance_h

#include <pcl/MetaParameter.h>
#include <pcl/ProcessImplementation.h>
#include <pcl/Image.h>
#include <pcl/Vector.h>

namespace pcl
{

class PSCMInstance : public ProcessImplementation
{
public:
   explicit PSCMInstance( const MetaProcess* );
   PSCMInstance( const PSCMInstance& );

   void Assign( const ProcessImplementation& ) override;
   UndoFlags UndoMode( const View& ) const override;
   bool CanExecuteOn( const View&, String& whyNot ) const override;
   bool ExecuteOn( View& ) override;
   bool ApplyToPreview( UInt16Image&, const DVector&, const DVector& ) const;
   void* LockParameter( const MetaParameter*, size_type tableRow ) override;

   pcl_enum imageType;
   double saturation;
   double protectBackground;
   pcl_bool spreadEnabled;
   double spectralSpread;
};

} // pcl

#endif // __PSCMInstance_h
