#ifndef __PSCMInterface_h
#define __PSCMInterface_h

#include <pcl/Button.h>
#include <pcl/CheckBox.h>
#include <pcl/ComboBox.h>
#include <pcl/Control.h>
#include <pcl/Label.h>
#include <pcl/NumericControl.h>
#include <pcl/ProcessInterface.h>
#include <pcl/Sizer.h>
#include <pcl/Thread.h>
#include <pcl/Timer.h>
#include <pcl/ToolButton.h>

#include "PSCMInstance.h"

namespace pcl
{

class PSCMInterface : public ProcessInterface
{
public:
   PSCMInterface();
   ~PSCMInterface() override;

   IsoString Id() const override;
   MetaProcess* Process() const override;
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
   InterfaceFeatures Features() const override;
   void ApplyInstance() const override;
   void ResetInstance() override;
   void RealTimePreviewUpdated( bool ) override;
   bool Launch( const MetaProcess&, const ProcessImplementation*, bool& dynamic, unsigned& flags ) override;
   ProcessImplementation* NewProcess() const override;
   bool ValidateProcess( const ProcessImplementation&, String& whyNot ) const override;
   bool RequiresInstanceValidation() const override;
   bool ImportProcess( const ProcessImplementation& ) override;
   bool RequiresRealTimePreviewUpdate( const UInt16Image&, const View&, const Rect&, int ) const override;
   bool GenerateRealTimePreview( UInt16Image&, const View&, const Rect&, int, String& ) const override;

private:
   class RealTimeThread : public Thread
   {
   public:
      UInt16Image m_image;
      bool m_success = false;

      RealTimeThread();
      void Reset( const UInt16Image&, const PSCMInstance&, const DVector&, const DVector& );
      void Run() override;

   private:
      PSCMInstance m_instance;
      DVector m_median;
      DVector m_mad;
   };

   struct GUIData
   {
      GUIData( PSCMInterface& );

      VerticalSizer Global_Sizer;
      Label Title_Label;
      Label Info_Label;
      HorizontalSizer ImageType_Sizer;
      Label ImageType_Label;
      ComboBox ImageType_ComboBox;
      HorizontalSizer Saturation_Sizer;
      NumericControl Saturation_NumericControl;
      ToolButton Saturation_ResetButton;
      HorizontalSizer ProtectBackground_Sizer;
      NumericControl ProtectBackground_NumericControl;
      ToolButton ProtectBackground_ResetButton;
      CheckBox SpreadEnabled_CheckBox;
      HorizontalSizer SpectralSpread_Sizer;
      NumericControl SpectralSpread_NumericControl;
      ToolButton SpectralSpread_ResetButton;
      Timer UpdateRealTimePreview_Timer;
   };

   PSCMInstance m_instance;
   GUIData* GUI = nullptr;
   mutable RealTimeThread* m_realTimeThread = nullptr;

   void UpdateControls();
   void UpdateRealTimePreview();
   void e_ImageType_Selected( ComboBox&, int );
   void e_NumericControl_ValueUpdated( NumericEdit&, double );
   void e_SpreadEnabled_Click( Button&, bool );
   void e_ResetButton_Click( Button&, bool );
   void e_UpdateRealTimePreview_Timer( Timer& );
};

extern PSCMInterface* ThePSCMInterface;

} // pcl

#endif // __PSCMInterface_h
