#include "PSCMProcess.h"
#include "PSCMInstance.h"
#include "PSCMInterface.h"
#if defined( PSCM_EMBED_ICON_SVG )
#include "PSCMIconSVG.h"
#endif
#if defined( PSCM_EMBED_ICON_XPM )
#include "PSCMIconXPM.h"
#endif

namespace pcl
{

PSCMProcess* ThePSCMProcess = nullptr;
PSCMImageTypeParameter* ThePSCMImageTypeParameter = nullptr;
PSCMSaturationParameter* ThePSCMSaturationParameter = nullptr;
PSCMProtectBackgroundParameter* ThePSCMProtectBackgroundParameter = nullptr;
PSCMSpreadEnabledParameter* ThePSCMSpreadEnabledParameter = nullptr;
PSCMSpectralSpreadParameter* ThePSCMSpectralSpreadParameter = nullptr;

PSCMProcess::PSCMProcess()
{
   ThePSCMProcess = this;
   new PSCMImageTypeParameter( this );
   new PSCMSaturationParameter( this );
   new PSCMProtectBackgroundParameter( this );
   new PSCMSpreadEnabledParameter( this );
   new PSCMSpectralSpreadParameter( this );
}

IsoString PSCMProcess::Id() const
{
   return "PlanckStarColorMapping";
}

IsoString PSCMProcess::Category() const
{
   return "ColorSpaces";
}

uint32 PSCMProcess::Version() const
{
   return 0x10300;
}

String PSCMProcess::Description() const
{
   return "<html><p>Maps stellar colors to blackbody colors using Planck's law.</p>"
          "<p>Use on linear three-channel images. Supports HOO, HSO, SHO, RGB, and "
          "RGB with the green channel ignored.</p></html>";
}

#if defined( PSCM_EMBED_ICON_SVG )
IsoString PSCMProcess::IconImageSVG() const
{
   return PSCMIconSVG();
}
#endif

String PSCMProcess::IconImageSVGFile() const
{
#if defined( PSCM_EMBED_ICON_SVG ) || defined( PSCM_EMBED_ICON_XPM )
   return String();
#else
   return "@module_icons_dir/PlanckStarColorMapping.svg";
#endif
}

#if defined( PSCM_EMBED_ICON_XPM )
const char** PSCMProcess::IconImageXPM() const
{
   return PSCMIconXPM();
}
#else
String PSCMProcess::IconImageFile() const
{
   return "D:/_User/Rainer/Documents/_Software/GitHub/PSCM/src/icons/PlanckColorBar_Q.png";
}

String PSCMProcess::SmallIconImageFile() const
{
   return "D:/_User/Rainer/Documents/_Software/GitHub/PSCM/src/icons/PlanckColorBar_Q.png";
}
#endif

ProcessInterface* PSCMProcess::DefaultInterface() const
{
   return ThePSCMInterface;
}

ProcessImplementation* PSCMProcess::Create() const
{
   return new PSCMInstance( this );
}

ProcessImplementation* PSCMProcess::Clone( const ProcessImplementation& p ) const
{
   const PSCMInstance* x = dynamic_cast<const PSCMInstance*>( &p );
   return (x != nullptr) ? new PSCMInstance( *x ) : nullptr;
}

PSCMImageTypeParameter::PSCMImageTypeParameter( MetaProcess* p ) : MetaEnumeration( p )
{
   ThePSCMImageTypeParameter = this;
}

IsoString PSCMImageTypeParameter::Id() const { return "imageType"; }
size_type PSCMImageTypeParameter::NumberOfElements() const { return NumberOfImageTypes; }

IsoString PSCMImageTypeParameter::ElementId( size_type i ) const
{
   static const char* ids[] = { "HOO", "HSO", "SHO", "RGB", "RGBIgnoreG" };
   if ( i < NumberOfImageTypes )
      return IsoString( ids[i] );
   return IsoString();
}

int PSCMImageTypeParameter::ElementValue( size_type i ) const
{
   return int( i );
}

size_type PSCMImageTypeParameter::DefaultValueIndex() const
{
   return Default;
}

PSCMSaturationParameter::PSCMSaturationParameter( MetaProcess* p ) : MetaDouble( p )
{
   ThePSCMSaturationParameter = this;
}

IsoString PSCMSaturationParameter::Id() const { return "colorSaturationFactor"; }
int PSCMSaturationParameter::Precision() const { return 2; }
double PSCMSaturationParameter::MinimumValue() const { return 0; }
double PSCMSaturationParameter::MaximumValue() const { return 2; }
double PSCMSaturationParameter::DefaultValue() const { return 1; }

PSCMProtectBackgroundParameter::PSCMProtectBackgroundParameter( MetaProcess* p ) : MetaDouble( p )
{
   ThePSCMProtectBackgroundParameter = this;
}

IsoString PSCMProtectBackgroundParameter::Id() const { return "protectBackground"; }
int PSCMProtectBackgroundParameter::Precision() const { return 1; }
double PSCMProtectBackgroundParameter::MinimumValue() const { return 0.5; }
double PSCMProtectBackgroundParameter::MaximumValue() const { return 12; }
double PSCMProtectBackgroundParameter::DefaultValue() const { return 1.5; }

PSCMSpreadEnabledParameter::PSCMSpreadEnabledParameter( MetaProcess* p ) : MetaBoolean( p )
{
   ThePSCMSpreadEnabledParameter = this;
}

IsoString PSCMSpreadEnabledParameter::Id() const { return "spreadSpectralClasses"; }
bool PSCMSpreadEnabledParameter::DefaultValue() const { return false; }

PSCMSpectralSpreadParameter::PSCMSpectralSpreadParameter( MetaProcess* p ) : MetaDouble( p )
{
   ThePSCMSpectralSpreadParameter = this;
}

IsoString PSCMSpectralSpreadParameter::Id() const { return "spectralSpread"; }
int PSCMSpectralSpreadParameter::Precision() const { return 2; }
double PSCMSpectralSpreadParameter::MinimumValue() const { return 1; }
double PSCMSpectralSpreadParameter::MaximumValue() const { return 2; }
double PSCMSpectralSpreadParameter::DefaultValue() const { return 1; }

} // pcl
