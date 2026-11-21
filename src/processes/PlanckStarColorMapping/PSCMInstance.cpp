#include "PSCMInstance.h"
#include "PSCMProcess.h"

#include <pcl/AutoViewLock.h>
#include <pcl/Console.h>
#include <pcl/Exception.h>
#include <pcl/Image.h>
#include <pcl/ImageVariant.h>
#include <pcl/StandardStatus.h>
#include <pcl/View.h>
#include <pcl/Vector.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace pcl
{
namespace
{

constexpr double LambdaSII = 672.4;
constexpr double LambdaHa = 656.3;
constexpr double LambdaOIII = 500.7;
constexpr double LambdaR = 622.0;
constexpr double LambdaG = 530.0;
constexpr double LambdaB = 476.0;

struct WavelengthPair
{
   int channel0;
   double lambda0;
   int channel1;
   double lambda1;
};

struct LCh
{
   double L;
   double C;
   double h;
   double saturationFactor;
};

struct RGB
{
   double r;
   double g;
   double b;
};

double Clamp( double x )
{
   return std::max( 0.0, std::min( 1.0, x ) );
}

double FinvLab( double t )
{
   const double t3 = t*t*t;
   return (t3 > 0.008856) ? t3 : (t - 16.0/116.0)/7.787;
}

double FLab( double t )
{
   return (t > 0.008856) ? std::cbrt( t ) : (7.787*t + 16.0/116.0);
}

LCh RGBToLCh( double r, double g, double b )
{
   double X = (0.4124564*r + 0.3575761*g + 0.1804375*b)/0.95047;
   double Y = 0.2126729*r + 0.7151522*g + 0.0721750*b;
   double Z = (0.0193339*r + 0.1191920*g + 0.9503041*b)/1.08883;

   const double fx = FLab( X );
   const double fy = FLab( Y );
   const double fz = FLab( Z );
   const double a = 500*(fx - fy);
   const double labB = 200*(fy - fz);
   double h = std::atan2( labB, a )*180.0/3.14159265358979323846;
   if ( h < 0 )
      h += 360;

   return { 116*fy - 16, std::sqrt( a*a + labB*labB ), h, 0 };
}

RGB LChToRGB( double L, double C, double h )
{
   const double hr = h*3.14159265358979323846/180.0;
   const double a = C*std::cos( hr );
   const double labB = C*std::sin( hr );
   const double fy = (L + 16)/116;
   const double fx = fy + a/500;
   const double fz = fy - labB/200;
   const double X = 0.95047*FinvLab( fx );
   const double Y = FinvLab( fy );
   const double Z = 1.08883*FinvLab( fz );

   return {
      Clamp(  3.2404542*X - 1.5371385*Y - 0.4985314*Z ),
      Clamp( -0.9692660*X + 1.8760108*Y + 0.0415560*Z ),
      Clamp(  0.0556434*X - 0.2040259*Y + 1.0572252*Z )
   };
}

LCh LChForRatio( double dbeta )
{
   const double logs[3] = { dbeta/LambdaR, dbeta/LambdaG, dbeta/LambdaB };
   const double maximum = std::max( logs[0], std::max( logs[1], logs[2] ) );
   const double r = std::exp( logs[0] - maximum );
   const double g = std::exp( logs[1] - maximum );
   const double b = std::exp( logs[2] - maximum );
   LCh lch = RGBToLCh( r, g, b );
   const double dRGB = (1.0 - r*g*b)*(1.0 - r*g*b);
   lch.saturationFactor = 1.0 - std::exp( -dRGB/1.0e-12 );
   return lch;
}

double SaturationCorrection( double dbeta, double chroma )
{
   const double neutralChroma = LChForRatio( dbeta ).C;
   const double correction = chroma/std::max( neutralChroma, 1.0e-3 );
   return (dbeta < 0) ? std::sqrt( correction ) : correction*correction;
}

std::vector<WavelengthPair> GetPairs( int type, const DVector& mad )
{
   std::vector<WavelengthPair> pairs;
   auto addPair = [&pairs]( int c0, double l0, int c1, double l1 )
   {
      pairs.push_back( { c0, l0, c1, l1 } );
   };

   auto orderByNoise = [&mad, &addPair]( int c0, double l0, int c1, double l1 )
   {
      if ( mad[c1] < mad[c0] )
         addPair( c1, l1, c0, l0 );
      else
         addPair( c0, l0, c1, l1 );
   };

   switch ( type )
   {
   case PSCMImageTypeParameter::HOO:
      orderByNoise( 0, LambdaHa, 2, LambdaOIII );
      break;
   case PSCMImageTypeParameter::HSO:
      orderByNoise( 0, LambdaHa, 2, LambdaOIII );
      orderByNoise( 1, LambdaSII, 2, LambdaOIII );
      break;
   case PSCMImageTypeParameter::SHO:
      orderByNoise( 0, LambdaSII, 2, LambdaOIII );
      orderByNoise( 1, LambdaHa, 2, LambdaOIII );
      break;
   case PSCMImageTypeParameter::RGB:
      orderByNoise( 0, LambdaR, 1, LambdaG );
      orderByNoise( 0, LambdaR, 2, LambdaB );
      break;
   case PSCMImageTypeParameter::RGBIgnoreG:
      orderByNoise( 0, LambdaR, 2, LambdaB );
      break;
   default:
      throw Error( "Unsupported PSCM image type." );
   }
   return pairs;
}

double BackgroundWeight( double signal, double mad, double protection )
{
   if ( mad <= 0 )
      return (signal == 0) ? 0 : 1;
   const double q = signal/(mad*protection);
   return 1.0 - std::exp( -q*q );
}

double RegularizedRatio( double numerator, double denominator, double weight )
{
   double ratio;
   if ( denominator == 0 )
      ratio = (numerator == 0) ? 1.0 : 1.0e3;
   else
      ratio = std::min( std::abs( numerator/denominator ), 1.0e3 );
   return weight*ratio + (1.0 - weight);
}

double PairVariance( double lambdaFactor, double ratio,
                     double mad0, double signal0, double mad1, double signal1 )
{
   const double epsilon = 1.0e-24;
   const double v = lambdaFactor*lambdaFactor/(ratio*ratio)
      *(mad0*mad0/std::max( signal0*signal0, epsilon )
       + mad1*mad1/std::max( signal1*signal1, epsilon ));
   return std::max( v, 1.0e-12 );
}

struct StatusCallbackReset
{
   explicit StatusCallbackReset( ImageVariant& i ) : image( i ) {}
   ~StatusCallbackReset()
   {
      image->SetStatusCallback( nullptr );
   }
   ImageVariant& image;
};

template <class P>
double SampleAsDouble( const GenericImage<P>& image, int x, int y, int channel )
{
   double value;
   P::FromSample( value, image( x, y, channel ) );
   return value;
}

template <class P>
void SetSampleFromDouble( GenericImage<P>& image, double value,
                          int x, int y, int channel )
{
   image( x, y, channel ) = P::ToSample( value );
}

template <class P>
void ApplyToImage( GenericImage<P>& image,
                   const DVector& median, const DVector& mad,
                   const std::vector<WavelengthPair>& pairs,
                   double saturation, double protection, double spread )
{
   for ( int y = 0; y < image.Height(); ++y )
      for ( int x = 0; x < image.Width(); ++x, ++image.Status() )
      {
         double deltaBeta[2] = { 0, 0 };
         double weights[2] = { 0, 0 };
         double variances[2] = { 0, 0 };

         for ( size_type k = 0; k < pairs.size(); ++k )
         {
            const WavelengthPair& pair = pairs[k];
            const double signal0 = SampleAsDouble( image, x, y, pair.channel0 ) - median[pair.channel0];
            const double signal1 = SampleAsDouble( image, x, y, pair.channel1 ) - median[pair.channel1];
            const double mad0 = mad[pair.channel0];
            const double mad1 = mad[pair.channel1];
            const double w = BackgroundWeight( signal0, mad0, protection );
            const double ratio = RegularizedRatio( signal1, signal0, w );
            const double lambdaFactor = pair.lambda0*pair.lambda1/(pair.lambda0 - pair.lambda1);
            deltaBeta[k] = std::log( ratio )*lambdaFactor;
            weights[k] = w;
            variances[k] = PairVariance( lambdaFactor, ratio, mad0, signal0, mad1, signal1 );
         }

         double dbeta = deltaBeta[0];
         double colorWeight = weights[0];
         if ( pairs.size() == 2 )
         {
            const double mix0 = variances[1]/(variances[0] + variances[1]);
            dbeta = mix0*deltaBeta[0] + (1.0 - mix0)*deltaBeta[1];
            colorWeight = std::min( weights[0], weights[1] );
         }

         const LCh target = LChForRatio( spread*dbeta );
         double cFactor = saturation*target.saturationFactor*colorWeight;
         if ( spread > 1 )
            cFactor *= SaturationCorrection( dbeta, target.C );

         const LCh original = RGBToLCh( SampleAsDouble( image, x, y, 0 ),
                                        SampleAsDouble( image, x, y, 1 ),
                                        SampleAsDouble( image, x, y, 2 ) );
         const RGB mapped = LChToRGB( original.L, cFactor*original.C, target.h );
         SetSampleFromDouble( image, mapped.r, x, y, 0 );
         SetSampleFromDouble( image, mapped.g, x, y, 1 );
         SetSampleFromDouble( image, mapped.b, x, y, 2 );
      }
}

} // anonymous namespace

PSCMInstance::PSCMInstance( const MetaProcess* m )
   : ProcessImplementation( m )
   , imageType( static_cast<pcl_enum>( ThePSCMImageTypeParameter->DefaultValueIndex() ) )
   , saturation( ThePSCMSaturationParameter->DefaultValue() )
   , protectBackground( ThePSCMProtectBackgroundParameter->DefaultValue() )
   , spreadEnabled( ThePSCMSpreadEnabledParameter->DefaultValue() )
   , spectralSpread( ThePSCMSpectralSpreadParameter->DefaultValue() )
{
}

PSCMInstance::PSCMInstance( const PSCMInstance& x )
   : ProcessImplementation( x )
{
   Assign( x );
}

void PSCMInstance::Assign( const ProcessImplementation& p )
{
   const PSCMInstance* x = dynamic_cast<const PSCMInstance*>( &p );
   if ( x != nullptr )
   {
      imageType = x->imageType;
      saturation = x->saturation;
      protectBackground = x->protectBackground;
      spreadEnabled = x->spreadEnabled;
      spectralSpread = x->spectralSpread;
   }
}

UndoFlags PSCMInstance::UndoMode( const View& ) const
{
   return UndoFlag::PixelData;
}

bool PSCMInstance::CanExecuteOn( const View& view, String& whyNot ) const
{
   if ( view.Image().IsComplexSample() )
   {
      whyNot = "PSCM cannot be executed on complex images.";
      return false;
   }
   if ( view.Image()->NumberOfChannels() < 3 )
   {
      whyNot = "PSCM requires an image with at least three channels.";
      return false;
   }
   whyNot.Clear();
   return true;
}

bool PSCMInstance::ExecuteOn( View& view )
{
   AutoViewLock lock( view );
   ImageVariant image = view.Image();
   if ( image.IsComplexSample() || image->NumberOfChannels() < 3 )
      throw Error( "PSCM requires a non-complex image with at least three channels." );

   const DVector median = view.ComputeOrFetchProperty( "Median" ).ToDVector();
   const DVector mad = view.ComputeOrFetchProperty( "MAD" ).ToDVector();
   if ( median.Length() < 3 || mad.Length() < 3 )
      throw Error( "Unable to calculate RGB median and MAD statistics." );

   const std::vector<WavelengthPair> pairs = GetPairs( imageType, mad );
   const double spread = spreadEnabled ? spectralSpread : 1.0;

   Console().EnableAbort();
   StandardStatus status;
   image->SetStatusCallback( &status );
   StatusCallbackReset resetStatusCallback( image );
   const size_type count = image.NumberOfPixels();
   image.Status().Initialize( "Planck Star Color Mapping", count );

   if ( image.IsFloatSample() )
      switch ( image.BitsPerSample() )
      {
      case 32: ApplyToImage( static_cast<pcl::Image&>( *image ), median, mad, pairs,
                             saturation, protectBackground, spread ); break;
      case 64: ApplyToImage( static_cast<pcl::DImage&>( *image ), median, mad, pairs,
                             saturation, protectBackground, spread ); break;
      default: throw Error( "Unsupported floating-point sample format." );
      }
   else
      switch ( image.BitsPerSample() )
      {
      case  8: ApplyToImage( static_cast<pcl::UInt8Image&>( *image ), median, mad, pairs,
                             saturation, protectBackground, spread ); break;
      case 16: ApplyToImage( static_cast<pcl::UInt16Image&>( *image ), median, mad, pairs,
                             saturation, protectBackground, spread ); break;
      case 32: ApplyToImage( static_cast<pcl::UInt32Image&>( *image ), median, mad, pairs,
                             saturation, protectBackground, spread ); break;
      default: throw Error( "Unsupported integer sample format." );
      }

   return true;
}

bool PSCMInstance::ApplyToPreview( UInt16Image& image,
                                   const DVector& median, const DVector& mad ) const
{
   if ( image.NumberOfChannels() < 3 || median.Length() < 3 || mad.Length() < 3 )
      return false;

   const std::vector<WavelengthPair> pairs = GetPairs( imageType, mad );
   const double spread = spreadEnabled ? spectralSpread : 1.0;
   image.Status().Initialize( "Planck Star Color Mapping Preview", image.NumberOfPixels() );
   ApplyToImage( image, median, mad, pairs, saturation, protectBackground, spread );
   return true;
}

void* PSCMInstance::LockParameter( const MetaParameter* p, size_type )
{
   if ( p == ThePSCMImageTypeParameter )
      return &imageType;
   if ( p == ThePSCMSaturationParameter )
      return &saturation;
   if ( p == ThePSCMProtectBackgroundParameter )
      return &protectBackground;
   if ( p == ThePSCMSpreadEnabledParameter )
      return &spreadEnabled;
   if ( p == ThePSCMSpectralSpreadParameter )
      return &spectralSpread;
   return nullptr;
}

} // pcl
