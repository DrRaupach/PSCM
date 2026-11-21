#ifndef __PSCMProcess_h
#define __PSCMProcess_h

#include <pcl/MetaProcess.h>
#include <pcl/MetaParameter.h>

namespace pcl
{

class PSCMProcess : public MetaProcess
{
public:
   PSCMProcess();
   IsoString Id() const override;
   IsoString Category() const override;
   uint32 Version() const override;
   String Description() const override;
#if defined( PSCM_EMBED_ICON_SVG )
   IsoString IconImageSVG() const override;
#endif
#if defined( PSCM_EMBED_ICON_XPM )
   const char** IconImageXPM() const override;
#else
   String IconImageFile() const override;
   String SmallIconImageFile() const override;
#endif
   String IconImageSVGFile() const override;
   ProcessInterface* DefaultInterface() const override;
   ProcessImplementation* Create() const override;
   ProcessImplementation* Clone( const ProcessImplementation& ) const override;
};

class PSCMImageTypeParameter : public MetaEnumeration
{
public:
   enum { HOO, HSO, SHO, RGB, RGBIgnoreG, NumberOfImageTypes, Default = HOO };
   PSCMImageTypeParameter( MetaProcess* );
   IsoString Id() const override;
   size_type NumberOfElements() const override;
   IsoString ElementId( size_type ) const override;
   int ElementValue( size_type ) const override;
   size_type DefaultValueIndex() const override;
};

class PSCMSaturationParameter : public MetaDouble
{
public:
   PSCMSaturationParameter( MetaProcess* );
   IsoString Id() const override;
   int Precision() const override;
   double MinimumValue() const override;
   double MaximumValue() const override;
   double DefaultValue() const override;
};

class PSCMProtectBackgroundParameter : public MetaDouble
{
public:
   PSCMProtectBackgroundParameter( MetaProcess* );
   IsoString Id() const override;
   int Precision() const override;
   double MinimumValue() const override;
   double MaximumValue() const override;
   double DefaultValue() const override;
};

class PSCMSpreadEnabledParameter : public MetaBoolean
{
public:
   PSCMSpreadEnabledParameter( MetaProcess* );
   IsoString Id() const override;
   bool DefaultValue() const override;
};

class PSCMSpectralSpreadParameter : public MetaDouble
{
public:
   PSCMSpectralSpreadParameter( MetaProcess* );
   IsoString Id() const override;
   int Precision() const override;
   double MinimumValue() const override;
   double MaximumValue() const override;
   double DefaultValue() const override;
};

extern PSCMProcess* ThePSCMProcess;
extern PSCMImageTypeParameter* ThePSCMImageTypeParameter;
extern PSCMSaturationParameter* ThePSCMSaturationParameter;
extern PSCMProtectBackgroundParameter* ThePSCMProtectBackgroundParameter;
extern PSCMSpreadEnabledParameter* ThePSCMSpreadEnabledParameter;
extern PSCMSpectralSpreadParameter* ThePSCMSpectralSpreadParameter;

} // pcl

#endif // __PSCMProcess_h
