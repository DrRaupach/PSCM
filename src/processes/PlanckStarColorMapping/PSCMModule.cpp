#include "PSCMProcess.h"
#include "PSCMInterface.h"

#include <pcl/MetaModule.h>

#define MODULE_VERSION_MAJOR     1
#define MODULE_VERSION_MINOR     3
#define MODULE_VERSION_REVISION  0
#define MODULE_VERSION_BUILD     1
#define MODULE_VERSION_LANGUAGE  eng

namespace pcl
{

class PSCMModule : public MetaModule
{
public:
   const char* Version() const override
   {
      return PCL_MODULE_VERSION( MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR,
                                 MODULE_VERSION_REVISION, MODULE_VERSION_BUILD,
                                 MODULE_VERSION_LANGUAGE );
   }

   IsoString Name() const override { return "PlanckStarColorMapping"; }
   String Description() const override { return "Planck Star Color Mapping process module"; }
   String Company() const override { return "Dr. Rainer Raupach"; }
   String Author() const override { return "Dr. Rainer Raupach"; }
   String Copyright() const override { return "Copyright (C) 2026 Dr. Rainer Raupach"; }
   String TradeMarks() const override { return "PixInsight"; }

   String OriginalFileName() const override
   {
#ifdef __PCL_WINDOWS
      return "PlanckStarColorMapping-pxm.dll";
#elif defined( __PCL_MACOSX )
      return "PlanckStarColorMapping-pxm.dylib";
#else
      return "PlanckStarColorMapping-pxm.so";
#endif
   }

   void GetReleaseDate( int& year, int& month, int& day ) const override
   {
      year = 2026;
      month = 11;
      day = 18;
   }
};

} // pcl

PCL_MODULE_EXPORT int InstallPixInsightModule( int mode )
{
   new pcl::PSCMModule;
   if ( mode == pcl::InstallMode::FullInstall )
   {
      new pcl::PSCMProcess;
      new pcl::PSCMInterface;
   }
   return 0;
}
