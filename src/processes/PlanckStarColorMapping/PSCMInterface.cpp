#include "PSCMInterface.h"
#include "PSCMProcess.h"
#if defined( PSCM_EMBED_ICON_SVG )
#include "PSCMIconSVG.h"
#endif
#if defined( PSCM_EMBED_ICON_XPM )
#include "PSCMIconXPM.h"
#endif

#include <pcl/MuteStatus.h>
#include <pcl/RealTimePreview.h>

namespace pcl
{

PSCMInterface* ThePSCMInterface = nullptr;

namespace
{

struct PreviewStatusCallbackReset
{
   explicit PreviewStatusCallbackReset( UInt16Image& i ) : image( i ) {}
   ~PreviewStatusCallbackReset()
   {
      image.SetStatusCallback( nullptr );
   }
   UInt16Image& image;
};

} // anonymous namespace

PSCMInterface::PSCMInterface()
   : m_instance( ThePSCMProcess )
{
   ThePSCMInterface = this;
}

PSCMInterface::~PSCMInterface()
{
   if ( m_realTimeThread != nullptr )
   {
      m_realTimeThread->Abort();
      m_realTimeThread->Wait();
      delete m_realTimeThread;
      m_realTimeThread = nullptr;
   }
   delete GUI;
}

IsoString PSCMInterface::Id() const
{
   return "PlanckStarColorMapping";
}

MetaProcess* PSCMInterface::Process() const
{
   return ThePSCMProcess;
}

#if defined( PSCM_EMBED_ICON_SVG )
IsoString PSCMInterface::IconImageSVG() const
{
   return PSCMIconSVG();
}
#endif

String PSCMInterface::IconImageSVGFile() const
{
#if defined( PSCM_EMBED_ICON_SVG ) || defined( PSCM_EMBED_ICON_XPM )
   return String();
#else
   return "@module_icons_dir/PlanckStarColorMapping.svg";
#endif
}

#if defined( PSCM_EMBED_ICON_XPM )
const char** PSCMInterface::IconImageXPM() const
{
   return PSCMIconXPM();
}
#else
String PSCMInterface::IconImageFile() const
{
   return "D:/_User/Rainer/Documents/_Software/GitHub/PSCM/src/icons/PlanckColorBar_Q.png";
}

String PSCMInterface::SmallIconImageFile() const
{
   return "D:/_User/Rainer/Documents/_Software/GitHub/PSCM/src/icons/PlanckColorBar_Q.png";
}
#endif

InterfaceFeatures PSCMInterface::Features() const
{
   return InterfaceFeature::Default | InterfaceFeature::RealTimeButton;
}

void PSCMInterface::ApplyInstance() const
{
   m_instance.LaunchOnCurrentView();
}

void PSCMInterface::ResetInstance()
{
   PSCMInstance defaults( ThePSCMProcess );
   ImportProcess( defaults );
}

void PSCMInterface::RealTimePreviewUpdated( bool active )
{
   if ( active )
      RealTimePreview::SetOwner( *this );
   else
   {
      if ( m_realTimeThread != nullptr )
         m_realTimeThread->Abort();
      RealTimePreview::SetOwner( ProcessInterface::Null() );
   }
}

bool PSCMInterface::Launch( const MetaProcess& process,
                            const ProcessImplementation* instance,
                            bool& dynamic, unsigned& )
{
   if ( process.Id() != ThePSCMProcess->Id() )
      return false;

   if ( GUI == nullptr )
   {
      GUI = new GUIData( *this );
      SetWindowTitle( "Planck Star Color Mapping (PSCM)" );
   }

   dynamic = false;
   if ( instance != nullptr )
      m_instance.Assign( *instance );
   UpdateControls();
   UpdateRealTimePreview();
   return true;
}

ProcessImplementation* PSCMInterface::NewProcess() const
{
   return new PSCMInstance( m_instance );
}

bool PSCMInterface::ValidateProcess( const ProcessImplementation& p, String& whyNot ) const
{
   if ( dynamic_cast<const PSCMInstance*>( &p ) != nullptr )
   {
      whyNot.Clear();
      return true;
   }
   whyNot = "Not a Planck Star Color Mapping process instance.";
   return false;
}

bool PSCMInterface::RequiresInstanceValidation() const
{
   return true;
}

bool PSCMInterface::ImportProcess( const ProcessImplementation& p )
{
   const PSCMInstance* x = dynamic_cast<const PSCMInstance*>( &p );
   if ( x == nullptr )
      return false;
   m_instance.Assign( p );
   UpdateControls();
   UpdateRealTimePreview();
   return true;
}

bool PSCMInterface::RequiresRealTimePreviewUpdate( const UInt16Image&,
                                                   const View& view,
                                                   const Rect&, int ) const
{
   return !view.Image().IsComplexSample() && view.Image()->NumberOfChannels() >= 3;
}

PSCMInterface::RealTimeThread::RealTimeThread()
   : m_instance( ThePSCMProcess )
{
}

void PSCMInterface::RealTimeThread::Reset( const UInt16Image& image,
                                           const PSCMInstance& instance,
                                           const DVector& median,
                                           const DVector& mad )
{
   image.ResetSelections();
   m_image.Assign( image );
   m_instance.Assign( instance );
   m_median = median;
   m_mad = mad;
   m_success = false;
}

void PSCMInterface::RealTimeThread::Run()
{
   MuteStatus status;
   m_image.SetStatusCallback( &status );
   PreviewStatusCallbackReset resetStatusCallback( m_image );
   m_success = m_instance.ApplyToPreview( m_image, m_median, m_mad );
   m_image.ResetSelections();
}

bool PSCMInterface::GenerateRealTimePreview( UInt16Image& image,
                                             const View& view,
                                             const Rect&, int,
                                             String& info ) const
{
   info.Clear();
   if ( view.Image().IsComplexSample() || view.Image()->NumberOfChannels() < 3 )
   {
      info = "PSCM requires a non-complex image with at least three channels.";
      return false;
   }

   // The preview API supplies a const View, but property computation uses its
   // mutable cache interface.
   View& statisticsView = const_cast<View&>( view );
   const DVector median = statisticsView.ComputeOrFetchProperty( "Median" ).ToDVector();
   const DVector mad = statisticsView.ComputeOrFetchProperty( "MAD" ).ToDVector();
   if ( median.Length() < 3 || mad.Length() < 3 )
   {
      info = "Unable to calculate RGB median and MAD statistics.";
      return false;
   }

   if ( m_realTimeThread == nullptr )
      m_realTimeThread = new RealTimeThread;

   for ( ;; )
   {
      m_realTimeThread->Reset( image, m_instance, median, mad );
      m_realTimeThread->Start();

      while ( m_realTimeThread->IsActive() )
      {
         ProcessEvents();

         if ( !IsRealTimePreviewActive() )
         {
            m_realTimeThread->Abort();
            m_realTimeThread->Wait();
            delete m_realTimeThread;
            m_realTimeThread = nullptr;
            return false;
         }
      }

      if ( !m_realTimeThread->IsAborted() )
      {
         const bool success = m_realTimeThread->m_success;
         if ( success )
            image.Assign( m_realTimeThread->m_image );
         delete m_realTimeThread;
         m_realTimeThread = nullptr;

         if ( !success )
            info = "Unable to generate a PSCM preview for this image.";
         return success;
      }
   }
}

void PSCMInterface::UpdateControls()
{
   if ( GUI == nullptr )
      return;

   GUI->ImageType_ComboBox.SetCurrentItem( m_instance.imageType );
   GUI->Saturation_NumericControl.SetValue( m_instance.saturation );
   GUI->ProtectBackground_NumericControl.SetValue( m_instance.protectBackground );
   GUI->SpreadEnabled_CheckBox.SetChecked( m_instance.spreadEnabled );
   GUI->SpectralSpread_NumericControl.SetValue( m_instance.spectralSpread );
   GUI->SpectralSpread_NumericControl.Enable( m_instance.spreadEnabled );
}

void PSCMInterface::UpdateRealTimePreview()
{
   if ( IsRealTimePreviewActive() && GUI != nullptr )
   {
      if ( m_realTimeThread != nullptr )
         m_realTimeThread->Abort();
      GUI->UpdateRealTimePreview_Timer.Start();
   }
}

void PSCMInterface::e_ImageType_Selected( ComboBox&, int item )
{
   m_instance.imageType = item;
   UpdateRealTimePreview();
}

void PSCMInterface::e_NumericControl_ValueUpdated( NumericEdit& sender, double value )
{
   if ( sender == GUI->Saturation_NumericControl )
      m_instance.saturation = value;
   else if ( sender == GUI->ProtectBackground_NumericControl )
      m_instance.protectBackground = value;
   else if ( sender == GUI->SpectralSpread_NumericControl )
      m_instance.spectralSpread = value;
   UpdateRealTimePreview();
}

void PSCMInterface::e_SpreadEnabled_Click( Button&, bool checked )
{
   m_instance.spreadEnabled = checked;
   GUI->SpectralSpread_NumericControl.Enable( checked );
   UpdateRealTimePreview();
}

void PSCMInterface::e_ResetButton_Click( Button& sender, bool )
{
   if ( sender == GUI->Saturation_ResetButton )
   {
      m_instance.saturation = ThePSCMSaturationParameter->DefaultValue();
      GUI->Saturation_NumericControl.SetValue( m_instance.saturation );
   }
   else if ( sender == GUI->ProtectBackground_ResetButton )
   {
      m_instance.protectBackground = ThePSCMProtectBackgroundParameter->DefaultValue();
      GUI->ProtectBackground_NumericControl.SetValue( m_instance.protectBackground );
   }
   else if ( sender == GUI->SpectralSpread_ResetButton )
   {
      m_instance.spectralSpread = ThePSCMSpectralSpreadParameter->DefaultValue();
      GUI->SpectralSpread_NumericControl.SetValue( m_instance.spectralSpread );
   }
   UpdateRealTimePreview();
}

void PSCMInterface::e_UpdateRealTimePreview_Timer( Timer& )
{
   if ( m_realTimeThread != nullptr && m_realTimeThread->IsActive() )
      return;

   if ( IsRealTimePreviewActive() )
      RealTimePreview::Update();
}

PSCMInterface::GUIData::GUIData( PSCMInterface& w )
   : Title_Label( String(), w )
   , Info_Label( String(), w )
   , ImageType_Label( String(), w )
   , ImageType_ComboBox( w )
   , Saturation_NumericControl( w )
   , Saturation_ResetButton( String(), Bitmap::Null(), false, w )
   , ProtectBackground_NumericControl( w )
   , ProtectBackground_ResetButton( String(), Bitmap::Null(), false, w )
   , SpreadEnabled_CheckBox( String(), w )
   , SpectralSpread_NumericControl( w )
   , SpectralSpread_ResetButton( String(), Bitmap::Null(), false, w )
{
   const int labelWidth = w.Font().Width( String( "Protect Background (X*MAD):" ) + 'M' );
   const int spacing = w.LogicalPixelsToPhysical( 6 );

   Title_Label.EnableWordWrapping();
   Title_Label.EnableRichText();
   Title_Label.SetStyleSheet( w.ScaledStyleSheet(
      "QLabel { border: 1px solid gray; padding: 0.25em; }" ) );
   Title_Label.SetText(
      "<p><strong>Planck Star Color Mapping (PSCM) version V1.3.0</strong><br/>"
      "Copyright &copy; 2026 Dr. Rainer Raupach</p>" );

   Info_Label.EnableWordWrapping();
   String infoText =
      "How to use PSCM?\n"
      "(example/typical workflow for HOO)\n\n"
      "1.  Load matching LINEAR Ha and OIII images after DBE/Graxpert.\n\n"
      "2.  Combine to HOO color image using ChannelCombination.\n\n"
      "3.  Apply ImageSolver to find astrometric solution on HOO image.\n\n"
      "4.  Apply SPCC with 'Red filter' at 656.3, 'Green/Blue filter' at 500.7 in \n"
      "     'Narrowband mode' and 'Optimize for Stars' checked. The white reference \n"
      "     should not be ''too hot''. 'Average Galaxy' (~4500K) is a good choice.\n\n"
      "5.  Derive the Starless image (e.g. by SXT), also the Stars in unscreen mode.\n"
      "     [Stars can also be calculated manually using PixelMath by ~(~HOO / ~Starless)]\n\n"
      "6.  Apply PSCM to the Star image which transforms the HOO colors\n"
      "     to black body colors according to the stars' temperatures.\n\n"
      "7.  Combine the PSCM mapped Stars with the Starless by screening,\n"
      "     e.g. using PixelMath by ~(~Starless * ~Stars)\n\n";
   infoText += String::UTF8ToUTF16( "Voil\xC3\xA0!" );
   infoText +=
      " You now have a bi-color HOO image with (almost) naturally colored stars\n"
      "without the need of an additional RGB image, still in lineal domain.\n"
      "Continue with stretching and further post-processing as usual.\n\n"
      "In general: the wavelengths in SPCC for S|H|O should be 672.4|656.3|500.7\n"
      "assigned to the respective filter slot. For RGB, the standard filters can be used.\n";
   Info_Label.SetText( infoText );

   ImageType_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );
   ImageType_Label.SetText( "Image Input Type:" );
   ImageType_Label.SetFixedWidth( labelWidth );
   ImageType_ComboBox.AddItem( "HOO" );
   ImageType_ComboBox.AddItem( "HSO" );
   ImageType_ComboBox.AddItem( "SHO" );
   ImageType_ComboBox.AddItem( "RGB" );
   ImageType_ComboBox.AddItem( "RGB (ignore G)" );
   ImageType_ComboBox.OnItemSelected(
      (ComboBox::item_event_handler)&PSCMInterface::e_ImageType_Selected, w );
   ImageType_Sizer.SetSpacing( spacing );
   ImageType_Sizer.Add( ImageType_Label );
   ImageType_Sizer.Add( ImageType_ComboBox, 100 );

   Saturation_NumericControl.label.SetText( "Color Saturation Factor:" );
   Saturation_NumericControl.label.SetFixedWidth( labelWidth );
   Saturation_NumericControl.SetReal();
   Saturation_NumericControl.SetRange( 0, 2 );
   Saturation_NumericControl.SetPrecision( 2 );
   Saturation_NumericControl.slider.SetRange( 0, 200 );
   Saturation_NumericControl.slider.SetScaledMinWidth( 250 );
   Saturation_NumericControl.SetToolTip( "Increased saturation may increase color errors in the input image." );
   Saturation_NumericControl.OnValueUpdated(
      (NumericEdit::value_event_handler)&PSCMInterface::e_NumericControl_ValueUpdated, w );
   Saturation_ResetButton.SetIcon( Bitmap( w.ScaledResource( ":/icons/clear.png" ) ) );
   Saturation_ResetButton.SetScaledFixedSize( 20, 20 );
   Saturation_ResetButton.SetFocusStyle( FocusStyle::NoFocus );
   Saturation_ResetButton.SetToolTip( "Restore the factory default value (1.00)." );
   Saturation_ResetButton.OnClick(
      (Button::click_event_handler)&PSCMInterface::e_ResetButton_Click, w );
   Saturation_Sizer.SetSpacing( spacing );
   Saturation_Sizer.Add( Saturation_NumericControl, 100 );
   Saturation_Sizer.Add( Saturation_ResetButton );

   ProtectBackground_NumericControl.label.SetText( "Protect Background (X*MAD):" );
   ProtectBackground_NumericControl.label.SetFixedWidth( labelWidth );
   ProtectBackground_NumericControl.SetReal();
   ProtectBackground_NumericControl.SetRange( 0.5, 12 );
   ProtectBackground_NumericControl.SetPrecision( 1 );
   ProtectBackground_NumericControl.slider.SetRange( 0, 100 );
   ProtectBackground_NumericControl.slider.SetScaledMinWidth( 250 );
   ProtectBackground_NumericControl.SetToolTip( "Multiple of the background noise (MAD)." );
   ProtectBackground_NumericControl.OnValueUpdated(
      (NumericEdit::value_event_handler)&PSCMInterface::e_NumericControl_ValueUpdated, w );
   ProtectBackground_ResetButton.SetIcon( Bitmap( w.ScaledResource( ":/icons/clear.png" ) ) );
   ProtectBackground_ResetButton.SetScaledFixedSize( 20, 20 );
   ProtectBackground_ResetButton.SetFocusStyle( FocusStyle::NoFocus );
   ProtectBackground_ResetButton.SetToolTip( "Restore the factory default value (1.5)." );
   ProtectBackground_ResetButton.OnClick(
      (Button::click_event_handler)&PSCMInterface::e_ResetButton_Click, w );
   ProtectBackground_Sizer.SetSpacing( spacing );
   ProtectBackground_Sizer.Add( ProtectBackground_NumericControl, 100 );
   ProtectBackground_Sizer.Add( ProtectBackground_ResetButton );

   SpreadEnabled_CheckBox.SetText( "Unphysical" );
   SpreadEnabled_CheckBox.OnClick(
      (Button::click_event_handler)&PSCMInterface::e_SpreadEnabled_Click, w );

   SpectralSpread_NumericControl.label.SetText( "Spread Spectral Classes by:" );
   SpectralSpread_NumericControl.label.SetFixedWidth( labelWidth );
   SpectralSpread_NumericControl.SetReal();
   SpectralSpread_NumericControl.SetRange( 1, 2 );
   SpectralSpread_NumericControl.SetPrecision( 2 );
   SpectralSpread_NumericControl.slider.SetRange( 0, 100 );
   SpectralSpread_NumericControl.slider.SetScaledMinWidth( 250 );
   SpectralSpread_NumericControl.SetToolTip(
      "Spreads stellar temperatures relative to the white reference." );
   SpectralSpread_NumericControl.OnValueUpdated(
      (NumericEdit::value_event_handler)&PSCMInterface::e_NumericControl_ValueUpdated, w );
   SpectralSpread_ResetButton.SetIcon( Bitmap( w.ScaledResource( ":/icons/clear.png" ) ) );
   SpectralSpread_ResetButton.SetScaledFixedSize( 20, 20 );
   SpectralSpread_ResetButton.SetFocusStyle( FocusStyle::NoFocus );
   SpectralSpread_ResetButton.SetToolTip( "Restore the factory default value (1.00)." );
   SpectralSpread_ResetButton.OnClick(
      (Button::click_event_handler)&PSCMInterface::e_ResetButton_Click, w );
   SpectralSpread_Sizer.SetSpacing( spacing );
   SpectralSpread_Sizer.Add( SpectralSpread_NumericControl, 100 );
   SpectralSpread_Sizer.Add( SpectralSpread_ResetButton );

   Global_Sizer.SetMargin( 10 );
   Global_Sizer.SetSpacing( spacing );
   Global_Sizer.Add( Title_Label );
   Global_Sizer.Add( Info_Label );
   Global_Sizer.Add( ImageType_Sizer );
   Global_Sizer.Add( Saturation_Sizer );
   Global_Sizer.Add( ProtectBackground_Sizer );
   Global_Sizer.Add( SpreadEnabled_CheckBox );
   Global_Sizer.Add( SpectralSpread_Sizer );
   w.SetSizer( Global_Sizer );
   w.AdjustToContents();

   UpdateRealTimePreview_Timer.SetSingleShot();
   UpdateRealTimePreview_Timer.SetInterval( 0.025 );
   UpdateRealTimePreview_Timer.OnTimer(
      (Timer::timer_event_handler)&PSCMInterface::e_UpdateRealTimePreview_Timer, w );
}

} // pcl
