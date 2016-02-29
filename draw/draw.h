#ifndef __DRAW_H_
#define __DRAW_H_

#include <iostream>  // for ostream, <<, etc.
#include <cstdlib>   // for rand, etc.
#include <cfloat>    // for FLT_MAX, etc.
#include <cmath>     // for sqrt, etc.
#include <vector>    // for std::vector
#include <cassert>   // for assert
#include <limits>    // for numeric_limits

// ----------------------------------------------------------------------------
// For the sake of Windows, apparently this is a "Linux/Unix thing"


#ifndef OPENSTEER_M_PI
#define OPENSTEER_M_PI 3.14159265358979323846f
#endif

#ifdef _MSC_VER
#undef min
#undef max
#endif

#if defined (_XBOX)
    #include <xtl.h>
#elif defined (_WIN32)
    #include <windows.h>
#endif


// 简单的跨平台绘制工具包
namespace Draw {

    // 前置声明
    class Color;
    class Vec3;
    class Camera;
    class App;
    class Clock;
    class PlugIn;
    class AbstractVehicle;
    
    typedef std::size_t size_t;
    typedef std::ptrdiff_t ptrdiff_t;

    // ----------------------------------------------------------------------------
    // do all initialization related to graphics
    void initializeGraphics (int argc, char **argv);

    // ----------------------------------------------------------------------------
    // run graphics event loop
    void runGraphics (void);


    // ----------------------------------------------------------------------------
    // accessors for GLUT's window dimensions
    float drawGetWindowHeight (void);
    float drawGetWindowWidth (void);

    // ----------------------------------------------------------------------------
    // Generic interpolation


    template<class T> inline T interpolate (float alpha, const T& x0, const T& x1)
    {
        return x0 + ((x1 - x0) * alpha);
    }


    // ----------------------------------------------------------------------------
    // Random number utilities


    // Returns a float randomly distributed between 0 and 1

    inline float frandom01 (void)
    {
        return (((float) rand ()) / ((float) RAND_MAX));
    }


    // Returns a float randomly distributed between lowerBound and upperBound

    inline float frandom2 (float lowerBound, float upperBound)
    {
        return lowerBound + (frandom01 () * (upperBound - lowerBound));
    }


    // ----------------------------------------------------------------------------
    // Constrain a given value (x) to be between two (ordered) bounds: min
    // and max.  Returns x if it is between the bounds, otherwise returns
    // the nearer bound.


    inline float clip (const float x, const float min, const float max)
    {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }


    // ----------------------------------------------------------------------------
    // remap a value specified relative to a pair of bounding values
    // to the corresponding value relative to another pair of bounds.
    // Inspired by (dyna:remap-interval y y0 y1 z0 z1)


    inline float remapInterval (float x,
                                float in0, float in1,
                                float out0, float out1)
    {
        // uninterpolate: what is x relative to the interval in0:in1?
        float relative = (x - in0) / (in1 - in0);

        // now interpolate between output interval based on relative x
        return interpolate (relative, out0, out1);
    }


    // Like remapInterval but the result is clipped to remain between
    // out0 and out1


    inline float remapIntervalClip (float x,
                                    float in0, float in1,
                                    float out0, float out1)
    {
        // uninterpolate: what is x relative to the interval in0:in1?
        float relative = (x - in0) / (in1 - in0);

        // now interpolate between output interval based on relative x
        return interpolate (clip (relative, 0, 1), out0, out1);
    }


    // ----------------------------------------------------------------------------
    // classify a value relative to the interval between two bounds:
    //     returns -1 when below the lower bound
    //     returns  0 when between the bounds (inside the interval)
    //     returns +1 when above the upper bound


    inline int intervalComparison (float x, float lowerBound, float upperBound)
    {
        if (x < lowerBound) return -1;
        if (x > upperBound) return +1;
        return 0;
    }



    // ----------------------------------------------------------------------------


    inline float scalarRandomWalk (const float initial, 
                                   const float walkspeed,
                                   const float min,
                                   const float max)
    {
        const float next = initial + (((frandom01() * 2) - 1) * walkspeed);
        if (next < min) return min;
        if (next > max) return max;
        return next;
    }


    // ----------------------------------------------------------------------------


    inline float square (float x)
    {
        return x * x;
    }


    // ----------------------------------------------------------------------------
    // for debugging: prints one line with a given C expression, an equals sign,
    // and the value of the expression.  For example "angle = 35.6"


    #define debugPrint(e) (std::cout << #e" = " << (e) << std::endl << std::flush)


    // ----------------------------------------------------------------------------
    // blends new values into an accumulator to produce a smoothed time series
    //
    // Modifies its third argument, a reference to the float accumulator holding
    // the "smoothed time series."
    //
    // The first argument (smoothRate) is typically made proportional to "dt" the
    // simulation time step.  If smoothRate is 0 the accumulator will not change,
    // if smoothRate is 1 the accumulator will be set to the new value with no
    // smoothing.  Useful values are "near zero".
    //
    // Usage:
    //         blendIntoAccumulator (dt * 0.4f, currentFPS, smoothedFPS);


    template<class T>
    inline void blendIntoAccumulator (const float smoothRate,
                                      const T& newValue,
                                      T& smoothedAccumulator)
    {
        smoothedAccumulator = interpolate (clip (smoothRate, 0, 1),
                                           smoothedAccumulator,
                                           newValue);
    }


    // ----------------------------------------------------------------------------
    // given a new Angle and an old angle, adjust the new for angle wraparound (the
    // 0->360 flip), returning a value equivalent to newAngle, but closest in
    // absolute value to oldAngle.  For radians fullCircle = OPENSTEER_M_PI*2, for degrees
    // fullCircle = 360.  Based on code in stuart/bird/fish demo's camera.cc
    //
    // (not currently used)

    /*
      inline float distance1D (const float a, const float b)
      {
          const float d = a - b;
          return (d > 0) ? d : -d;
      }


      float adjustForAngleWraparound (float newAngle,
                                      float oldAngle,
                                      float fullCircle)
      {
          // adjust newAngle for angle wraparound: consider its current value (a)
          // as well as the angle 2pi larger (b) and 2pi smaller (c).  Select the
          // one closer (magnitude of difference) to the current value of oldAngle.
          const float a = newAngle;
          const float b = newAngle + fullCircle;
          const float c = newAngle - fullCircle;
          const float ad = distance1D (a, oldAngle);
          const float bd = distance1D (b, oldAngle);
          const float cd = distance1D (c, oldAngle);

          if ((bd < ad) && (bd < cd)) return b;
          if ((cd < ad) && (cd < bd)) return c;
          return a;
      }
    */


    // ----------------------------------------------------------------------------
    // Functions to encapsulate cross-platform differences for several <cmath>
    // functions.  Specifically, the C++ standard says that these functions are
    // in the std namespace (std::sqrt, etc.)  Apparently the MS VC6 compiler (or
    // its header files) do not implement this correctly and the function names
    // are in the global namespace.  We hope these -XXX versions are a temporary
    // expedient, to be removed later.


    #ifdef _WIN32

    inline float floorXXX (float x)          {return ::floor (x);}
    inline float  sqrtXXX (float x)          {return ::sqrt (x);}
    inline float   sinXXX (float x)          {return ::sin (x);}
    inline float   cosXXX (float x)          {return ::cos (x);}
    inline float   absXXX (float x)          {return ::abs (x);}
    inline int     absXXX (int x)            {return ::abs (x);}
    inline float   maxXXX (float x, float y) {if (x > y) return x; else return y;}
    inline float   minXXX (float x, float y) {if (x < y) return x; else return y;}

    #else

    inline float floorXXX (float x)          {return std::floor (x);}
    inline float  sqrtXXX (float x)          {return std::sqrt (x);}
    inline float   sinXXX (float x)          {return std::sin (x);}
    inline float   cosXXX (float x)          {return std::cos (x);}
    inline float   absXXX (float x)          {return std::abs (x);}
    inline int     absXXX (int x)            {return std::abs (x);}
    inline float   maxXXX (float x, float y) {return std::max (x, y);}
    inline float   minXXX (float x, float y) {return std::min (x, y);}

    #endif


    // ----------------------------------------------------------------------------
    // round (x)  "round off" x to the nearest integer (as a float value)
    //
    // This is a Gnu-sanctioned(?) post-ANSI-Standard(?) extension (as in
    // http://www.opengroup.org/onlinepubs/007904975/basedefs/math.h.html)
    // which may not be present in all C++ environments.  It is defined in
    // math.h headers in Linux and Mac OS X, but apparently not in Win32:


    #ifdef _WIN32

    inline float round (float x)
    {
      if (x < 0)
          return -floorXXX (0.5f - x);
      else
          return  floorXXX (0.5f + x);
    }

    #else 
    
    inline float round( float x )
    {
        return ::round( x );
    }
    
    #endif

    
    /**
     * Returns @a valueToClamp clamped to the range @a minValue - @a maxValue.
     */
    template< typename T >
    T
    clamp( T const& valueToClamp, T const& minValue, T const& maxValue) {
        assert( minValue <= maxValue && "minValue must be lesser or equal to maxValue."  );
        
        if ( valueToClamp < minValue ) {
            return minValue;
        } else if ( valueToClamp > maxValue ) {
            return maxValue;
        }
        
        return valueToClamp;
    }
    
    
    /**
     * Returns the floating point remainder of the division of @a x by @a y.
     * If @a y is @c 0 the behavior is undefined.
     */
    inline float modulo( float x, float y ) {
        assert( 0.0f != y && "Division by zero." );
        return std::fmod( x, y );
    }
    
    /**
     * Returns the floating point remainder of the division of @a x by @a y.
     * If @a y is @c 0 the behavior is undefined.
     */
    inline double modulo( double x, double y ) {
        assert( 0.0 != y && "Division by zero." );
        return std::fmod( x, y );
    }    
    
    /**
     * Returns the floating point remainder of the division of @a x by @a y.
     * If @a y is @c 0 the behavior is undefined.
     */
    inline long double modulo( long double x, long double y ) {
        assert( 0.0 != y && "Division by zero." );
        return std::fmod( x, y );
    }
    
    /**
     * Returns the floating point remainder of the division of @a x by @a y.
     * If @a y is @c 0 the behavior is undefined.
     */
    inline short modulo( short x, short y ) {
        assert( 0 != y && "Division by zero." );
        return x % y;
    }
    
    /**
     * Returns the floating point remainder of the division of @a x by @a y.
     * If @a y is @c 0 the behavior is undefined.
     */
    inline int modulo( int x, int y ) {
        assert( 0 != y && "Division by zero." );
        return x % y;
    }
    
    /**
     * Returns the floating point remainder of the division of @a x by @a y.
     * If @a y is @c 0 the behavior is undefined.
     */
    inline long modulo( long x, long y ) {
        assert( 0 != y && "Division by zero." );
        return x % y;
    }
    
    
    /**
     * Returns <code>value</code> if <code>value >= 0 </code>, otherwise
     * <code>-value</code>.
     */
    template< typename T >
    T abs( T const& value ) {
        return absXXX( value );
    }
    
    /**
     * Returns the maximum of the three values @a v0, @a v1, and @a v2.
     *
     * @todo Write a unit test.
     */
    template< typename T >
    T
    max( T const& v0, T const& v1, T const& v2 ) {
        return maxXXX( v0, maxXXX( v1, v2 ) );
    }
    
    
    /**
     * Returns the minimum of the three values @a v0, @a v1, and @a v2.
     *
     * @todo Write a unit test.
     */
    template< typename T >
    T
    min( T const& v0, T const& v1, T const& v2 ) {
        return minXXX( v0, minXXX( v1, v2 ) );
    }
    
    
    /**
     * Compares the absolute value of @a v with @a tolerance.
     *
     * See Christer Ericson, Real-Time Collision Detection, Morgan Kaufmann, 
     * 2005, pp. 441--443.
     *
     * @todo Write a unit test.
     */
    template< typename T >
    bool
    isZero( T const& v, T const& tolerance = std::numeric_limits< T >::epsilon() ) {
        return abs( v ) <= tolerance;
    }
    
    
    /**
     * Compares @a lhs with @a rhs given a specific @a tolerance.
     *
     * @attention Adapt @a tolerance to the range of values of @a lhs and 
     * @a rhs.
     * See Christer Ericson, Real-Time Collision Detection, Morgan Kaufmann, 
     * 2005, pp. 441--443.
     *
     * @return <code>abs( lhs - rhs ) <= tolerance</code>
     *
     * @todo Write a unit test.
     */
    template< typename T >
    bool
    equalsAbsolute( T const& lhs, T const& rhs, T const& tolerance = std::numeric_limits< T >::epsilon()  ) {
        return isZero( lhs - rhs, tolerance );
    }
    
    
    /**
     * Compares @a lhs with @a rhs given a specific @a tolerance taking the 
     * range of values into account.
     *
     * See Christer Ericson, Real-Time Collision Detection, Morgan Kaufmann, 
     * 2005, pp. 441--443.
     *
     * @return <code>abs( lhs - rhs ) <= tolerance * max( abs( lhs ), abs( rhs ), 1 )</code>
     *
     * @todo Write a unit test.
     */
    template< typename T >
    bool
    equalsRelative( T const& lhs, T const& rhs, T const& tolerance = std::numeric_limits< T >::epsilon()  ) {
        return isZero( lhs - rhs, tolerance * max( abs( lhs ), abs( rhs ), T( 1 ) ) );
    }
    
    
    /**
     * Approximately compares @a lhs with @a rhs given a specific @a tolerance  
     * taking the range of values into account.
     *
     * See Christer Ericson, Real-Time Collision Detection, Morgan Kaufmann, 
     * 2005, pp. 441--443.
     *
     * @return <code>abs( lhs - rhs ) <= tolerance * ( abs( lhs ) + abs( rhs ) + 1 )</code>
     *
     * @todo Write a unit test.
     */
    template< typename T >
    bool
    equalsRelativeApproximately( T const& lhs, T const& rhs, T const& tolerance = std::numeric_limits< T >::epsilon()  ) {
        return isZero( lhs - rhs, tolerance * ( abs( lhs ) + abs( rhs ) + T( 1 ) ) );
    }    
    
    
    /**
     * Shrinks the capacity of a std::vector to fit its content.
     *
     * See Scott Meyer, Effective STL, Addison-Wesley, 2001, pp. 77--79.
     */
    template< typename T >
    void shrinkToFit( std::vector< T >& v ) {
        std::vector< T >( v ).swap( v );
    }

    // ----------------------------------------------------------------------------
    // --- Cross Platform Clock
    class Clock
    {
    public:

        // constructor
        Clock ();

        // update this clock, called exactly once per simulation step ("frame")
        void update (void);

        // returns the number of seconds of real time (represented as a float)
        // since the clock was first updated.
        float realTimeSinceFirstClockUpdate (void);

        // force simulation time ahead, ignoring passage of real time.
        // Used for App's "single step forward" and animation mode
        float advanceSimulationTimeOneFrame (void);
        void advanceSimulationTime (const float seconds);

        // "wait" until next frame time
        void frameRateSync (void);


        // main clock modes: variable or fixed frame rate, real-time or animation
        // mode, running or paused.
    private:
        // run as fast as possible, simulation time is based on real time
        bool variableFrameRateMode;

        // fixed frame rate (ignored when in variable frame rate mode) in
        // real-time mode this is a "target", in animation mode it is absolute
        int fixedFrameRate;

        // used for offline, non-real-time applications
        bool animationMode;

        // is simulation running or paused?
        bool paused;
    public:
        int getFixedFrameRate (void) {return fixedFrameRate;}
        int setFixedFrameRate (int ffr) {return fixedFrameRate = ffr;}

        bool getAnimationMode (void) {return animationMode;}
        bool setAnimationMode (bool am) {return animationMode = am;}

        bool getVariableFrameRateMode (void) {return variableFrameRateMode;}
        bool setVariableFrameRateMode (bool vfrm)
             {return variableFrameRateMode = vfrm;}

        bool togglePausedState (void) {return (paused = !paused);};
        bool getPausedState (void) {return paused;};
        bool setPausedState (bool newPS) {return paused = newPS;};


        // clock keeps track of "smoothed" running average of recent frame rates.
        // When a fixed frame rate is used, a running average of "CPU load" is
        // kept (aka "non-wait time", the percentage of each frame time (time
        // step) that the CPU is busy).
    private:
        float smoothedFPS;
        float smoothedUsage;
        void updateSmoothedRegisters (void)
        {
            const float rate = getSmoothingRate ();
            if (elapsedRealTime > 0)
                blendIntoAccumulator (rate, 1 / elapsedRealTime, smoothedFPS);
            if (! getVariableFrameRateMode ())
                blendIntoAccumulator (rate, getUsage (), smoothedUsage);
        }
    public:
        float getSmoothedFPS (void) const {return smoothedFPS;}
        float getSmoothedUsage (void) const {return smoothedUsage;}
        float getSmoothingRate (void) const
        {
            if (smoothedFPS == 0) return 1; else return elapsedRealTime * 1.5f;
        }
        float getUsage (void)
        {
            // run time per frame over target frame time (as a percentage)
            return ((100 * elapsedNonWaitRealTime) / (1.0f / fixedFrameRate));
        }


        // clock state member variables and public accessors for them
    private:
        // real "wall clock" time since launch
        float totalRealTime;

        // total time simulation has run
        float totalSimulationTime;

        // total time spent paused
        float totalPausedTime;

        // sum of (non-realtime driven) advances to simulation time
        float totalAdvanceTime;

        // interval since last simulation time
        // (xxx does this need to be stored in the instance? xxx)
        float elapsedSimulationTime;

        // interval since last clock update time 
        // (xxx does this need to be stored in the instance? xxx)
        float elapsedRealTime;

        // interval since last clock update,
        // exclusive of time spent waiting for frame boundary when targetFPS>0
        float elapsedNonWaitRealTime;
    public:
        float getTotalRealTime (void) {return totalRealTime;}
        float getTotalSimulationTime (void) {return totalSimulationTime;}
        float getTotalPausedTime (void) {return totalPausedTime;}
        float getTotalAdvanceTime (void) {return totalAdvanceTime;}
        float getElapsedSimulationTime (void) {return elapsedSimulationTime;}
        float getElapsedRealTime (void) {return elapsedRealTime;}
        float getElapsedNonWaitRealTime (void) {return elapsedNonWaitRealTime;}


    private:
        // "manually" advance clock by this amount on next update
        float newAdvanceTime;

        // "Calendar time" when this clock was first updated
    #ifdef _WIN32
        // from QueryPerformanceCounter on Windows
        LONGLONG basePerformanceCounter;
    #else
        // from gettimeofday on Linux and Mac OS X
        int baseRealTimeSec;
        int baseRealTimeUsec;
    #endif
    };
// Forward declaration. Full declaration in Vec3.h
    class Vec3;
    
    
    class Color {
    public:
        Color();
        explicit Color( float greyValue );
        Color( float rValue, float gValue, float bValue, float aValue = 1.0f );
        explicit Color( Vec3 const& vector );
        
        float r() const;
        float g() const;
        float b() const;
        float a() const;
        
        void setR( float value );
        void setG( float value );
        void setB( float value );
        void setA( float value );
        void set( float rValue, float gValue, float bValue, float aValue = 1.0f );
        
        Vec3 convertToVec3() const;
    
        // this is necessary so that graphics API's such as DirectX
        // requiring a pointer to colors can do their conversion
        // without a lot of copying.
        float const*const colorFloatArray() const { return &r_; }

        Color& operator+=( Color const& other );
        
        /**
         * @todo What happens if color components become negative?
         */
        Color& operator-=( Color const& other );
        
        /**
         * @todo What happens if color components become negative?
         */
        Color& operator*=( float factor );
        
        /**
         * @todo What happens if color components become negative?
         */
        Color& operator/=( float factor );
        
        
    private:
        float r_;
        float g_;
        float b_;
         float a_;  // provided for API's which require four components        
    }; // class Color
    
    
    Color operator+( Color const& lhs, Color const& rhs );
    
    /**
     * @todo What happens if color components become negative?
     */
    Color operator-( Color const& lhs, Color const& rhs );
    
    /**
     * @todo What happens if color components become negative?
     */
    Color operator*( Color const& lhs, float rhs );
    
    /**
     * @todo What happens if color components become negative?
     */
    Color operator*( float lhs, Color const& rhs );
    
    /**
     * @todo What happens if color components become negative?
     */
    Color operator/( Color const& lhs, float rhs );
    
    
    Color grayColor( float value );
    
    extern Color const gBlack;
    extern Color const gWhite; 
    extern Color const gRed; 
    extern Color const gGreen;
    extern Color const gBlue;
    extern Color const gYellow;
    extern Color const gCyan;
    extern Color const gMagenta;
    extern Color const gOrange;
    extern Color const gDarkRed;
    extern Color const gDarkGreen;
    extern Color const gDarkBlue;
    extern Color const gDarkYellow;
    extern Color const gDarkCyan;
    extern Color const gDarkMagenta;
    extern Color const gDarkOrange;
    
    extern Color const gGray10;
    extern Color const gGray20;
    extern Color const gGray30;
    extern Color const gGray40;
    extern Color const gGray50;
    extern Color const gGray60;
    extern Color const gGray70;
    extern Color const gGray80;
    extern Color const gGray90;
// ----------------------------------------------------------------------------


    class Vec3
    {
    public:

        // ----------------------------------------- generic 3d vector operations

        // three-dimensional Cartesian coordinates
        float x, y, z;

        // constructors
        Vec3 (void): x( 0.0f ), y( 0.0f ), z( 0.0f ) {}
        Vec3 (float X, float Y, float Z) : x( X ), y( Y ), z( Z ) {}

        // vector addition
        Vec3 operator+ (const Vec3& v) const {return Vec3 (x+v.x, y+v.y, z+v.z);}

        // vector subtraction
        Vec3 operator- (const Vec3& v) const {return Vec3 (x-v.x, y-v.y, z-v.z);}

        // unary minus
        Vec3 operator- (void) const {return Vec3 (-x, -y, -z);}

        // vector times scalar product (scale length of vector times argument)
        Vec3 operator* (const float s) const {return Vec3 (x * s, y * s, z * s);}

        // vector divided by a scalar (divide length of vector by argument)
        Vec3 operator/ (const float s) const {return Vec3 (x / s, y / s, z / s);}

        // dot product
        float dot (const Vec3& v) const {return (x * v.x) + (y * v.y) + (z * v.z);}

        // length
        float length (void) const {return sqrtXXX (lengthSquared ());}

        // length squared
        float lengthSquared (void) const {return this->dot (*this);}

        // normalize: returns normalized version (parallel to this, length = 1)
        Vec3 normalize (void) const
        {
            // skip divide if length is zero
            const float len = length ();
            return (len>0) ? (*this)/len : (*this);
        }

        // cross product (modify "*this" to be A x B)
        // [XXX  side effecting -- deprecate this function?  XXX]
        void cross(const Vec3& a, const Vec3& b)
        {
            *this = Vec3 ((a.y * b.z) - (a.z * b.y),
                          (a.z * b.x) - (a.x * b.z),
                          (a.x * b.y) - (a.y * b.x));
        }

        // assignment
        Vec3 operator= (const Vec3& v) {x=v.x; y=v.y; z=v.z; return *this;}

        // set XYZ coordinates to given three floats
        Vec3 set (const float _x, const float _y, const float _z)
        {x = _x; y = _y; z = _z; return *this;}

        // +=
        Vec3 operator+= (const Vec3& v) {return *this = (*this + v);}

        // -=
        Vec3 operator-= (const Vec3& v) {return *this = (*this - v);}

        // *=
        Vec3 operator*= (const float& s) {return *this = (*this * s);}

        
        Vec3 operator/=( float d ) { return *this = (*this / d);  }
        
        // equality/inequality
        bool operator== (const Vec3& v) const {return x==v.x && y==v.y && z==v.z;}
        bool operator!= (const Vec3& v) const {return !(*this == v);}

        // @todo Remove - use @c distance from the Vec3Utilitites header instead.
        // XXX experimental (4-1-03 cwr): is this the right approach?  defining
        // XXX "Vec3 distance (vec3, Vec3)" collided with STL's distance template.
        static float distance (const Vec3& a, const Vec3& b){ return(a-b).length();}

        // --------------------------- utility member functions used in Draw

        // return component of vector parallel to a unit basis vector
        // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))

        inline Vec3 parallelComponent (const Vec3& unitBasis) const
        {
            const float projection = this->dot (unitBasis);
            return unitBasis * projection;
        }

        // return component of vector perpendicular to a unit basis vector
        // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))

        inline Vec3 perpendicularComponent (const Vec3& unitBasis) const
        {
            return (*this) - parallelComponent (unitBasis);
        }

        // clamps the length of a given vector to maxLength.  If the vector is
        // shorter its value is returned unaltered, if the vector is longer
        // the value returned has length of maxLength and is paralle to the
        // original input.

        Vec3 truncateLength (const float maxLength) const
        {
            const float maxLengthSquared = maxLength * maxLength;
            const float vecLengthSquared = this->lengthSquared ();
            if (vecLengthSquared <= maxLengthSquared)
                return *this;
            else
                return (*this) * (maxLength / sqrtXXX (vecLengthSquared));
        }

        // forces a 3d position onto the XZ (aka y=0) plane

        Vec3 setYtoZero (void) const {return Vec3 (this->x, 0, this->z);}

        // rotate this vector about the global Y (up) axis by the given angle

        Vec3 rotateAboutGlobalY (float angle) const 
        {
            const float s = sinXXX (angle);
            const float c = cosXXX (angle);
            return Vec3 ((this->x * c) + (this->z * s),
                         (this->y),
                         (this->z * c) - (this->x * s));
        }

        // version for caching sin/cos computation
        Vec3 rotateAboutGlobalY (float angle, float& sin, float& cos) const 
        {
            // is both are zero, they have not be initialized yet
            if (sin==0 && cos==0)
            {
                sin = sinXXX (angle);
                cos = cosXXX (angle);
            }
            return Vec3 ((this->x * cos) + (this->z * sin),
                         (this->y),
                         (this->z * cos) - (this->x * sin));
        }

        // if this position is outside sphere, push it back in by one diameter

        Vec3 sphericalWrapAround (const Vec3& center, float radius)
        {
            const Vec3 offset = *this - center;
            const float r = offset.length();
            if (r > radius)
                return *this + ((offset/r) * radius * -2);
            else
                return *this;
        }

        // names for frequently used vector constants
        static const Vec3 zero;
        static const Vec3 side;
        static const Vec3 up;
        static const Vec3 forward;
    };


    // ----------------------------------------------------------------------------
    // scalar times vector product ("float * Vec3")


    inline Vec3 operator* (float s, const Vec3& v) {return v*s;}


    // return cross product a x b
    inline Vec3 crossProduct(const Vec3& a, const Vec3& b)
    {
        Vec3 result((a.y * b.z) - (a.z * b.y),
                    (a.z * b.x) - (a.x * b.z),
                    (a.x * b.y) - (a.y * b.x));
        return result;
    }


    // ----------------------------------------------------------------------------
    // default character stream output method

#ifndef NOT_OPENSTEERDEMO  // only when building App

    inline std::ostream& operator<< (std::ostream& o, const Vec3& v)
    {
        return o << "(" << v.x << "," << v.y << "," << v.z << ")";
    }

#endif // NOT_OPENSTEERDEMO

    // ----------------------------------------------------------------------------
    // Returns a position randomly distributed inside a sphere of unit radius
    // centered at the origin.  Orientation will be random and length will range
    // between 0 and 1


    Vec3 RandomVectorInUnitRadiusSphere (void);


    // ----------------------------------------------------------------------------
    // Returns a position randomly distributed on a disk of unit radius
    // on the XZ (Y=0) plane, centered at the origin.  Orientation will be
    // random and length will range between 0 and 1


    Vec3 randomVectorOnUnitRadiusXZDisk (void);


    // ----------------------------------------------------------------------------
    // Returns a position randomly distributed on the surface of a sphere
    // of unit radius centered at the origin.  Orientation will be random
    // and length will be 1


    inline Vec3 RandomUnitVector (void)
    {
        return RandomVectorInUnitRadiusSphere().normalize();
    }


    // ----------------------------------------------------------------------------
    // Returns a position randomly distributed on a circle of unit radius
    // on the XZ (Y=0) plane, centered at the origin.  Orientation will be
    // random and length will be 1


    inline Vec3 RandomUnitVectorOnXZPlane (void)
    {
        return RandomVectorInUnitRadiusSphere().setYtoZero().normalize();
    }


    // ----------------------------------------------------------------------------
    // used by limitMaxDeviationAngle / limitMinDeviationAngle below


    Vec3 vecLimitDeviationAngleUtility (const bool insideOrOutside,
                                        const Vec3& source,
                                        const float cosineOfConeAngle,
                                        const Vec3& basis);


    // ----------------------------------------------------------------------------
    // Enforce an upper bound on the angle by which a given arbitrary vector
    // diviates from a given reference direction (specified by a unit basis
    // vector).  The effect is to clip the "source" vector to be inside a cone
    // defined by the basis and an angle.


    inline Vec3 limitMaxDeviationAngle (const Vec3& source,
                                        const float cosineOfConeAngle,
                                        const Vec3& basis)
    {
        return vecLimitDeviationAngleUtility (true, // force source INSIDE cone
                                              source,
                                              cosineOfConeAngle,
                                              basis);
    }


    // ----------------------------------------------------------------------------
    // Enforce a lower bound on the angle by which a given arbitrary vector
    // diviates from a given reference direction (specified by a unit basis
    // vector).  The effect is to clip the "source" vector to be outside a cone
    // defined by the basis and an angle.


    inline Vec3 limitMinDeviationAngle (const Vec3& source,
                                        const float cosineOfConeAngle,
                                        const Vec3& basis)
    {    
        return vecLimitDeviationAngleUtility (false, // force source OUTSIDE cone
                                              source,
                                              cosineOfConeAngle,
                                              basis);
    }


    // ----------------------------------------------------------------------------
    // Returns the distance between a point and a line.  The line is defined in
    // terms of a point on the line ("lineOrigin") and a UNIT vector parallel to
    // the line ("lineUnitTangent")


    inline float distanceFromLine (const Vec3& point,
                                   const Vec3& lineOrigin,
                                   const Vec3& lineUnitTangent)
    {
        const Vec3 offset = point - lineOrigin;
        const Vec3 perp = offset.perpendicularComponent (lineUnitTangent);
        return perp.length();
    }


    // ----------------------------------------------------------------------------
    // given a vector, return a vector perpendicular to it (note that this
    // arbitrarily selects one of the infinitude of perpendicular vectors)


    Vec3 findPerpendicularIn3d (const Vec3& direction);


    // ----------------------------------------------------------------------------
    // candidates for global utility functions
    //
    // dot
    // cross
    // length
    // distance
    // normalized

/**
     * Returns the nearest point on the segment @a segmentPoint0 to 
     * @a segmentPoint1 from @a point.
     */
    Draw::Vec3  nearestPointOnSegment( const Vec3& point,
                                            const Vec3& segmentPoint0,
                                            const Vec3& segmentPoint1 );
    
    /**
     * Computes minimum distance from @a point to the line segment defined by
     * @a segmentPoint0 and @a segmentPoint1.
     */
    float pointToSegmentDistance( const Vec3& point,
                                  const Vec3& segmentPoint0,
                                  const Vec3& segmentPoint1);
        
    /**
     * Retuns distance between @a a and @a b.
     */
    inline float distance (const Vec3& a, const Vec3& b) {
        return (a-b).length();
    } 
    
    
    /**
     * Elementwise relative tolerance comparison of @a lhs and @a rhs taking
     * the range of the elements into account.
     *
     * See Christer Ericson, Real-Time Collision Detection, Morgan Kaufmann, 
     * 2005, pp. 441--443.
     *
     * @todo Rewrite using the stl or providing an own range based function.
     */
    inline
    bool
    equalsRelative( Vec3 const& lhs, 
                     Vec3 const& rhs, 
                     float const& tolerance = std::numeric_limits< float >::epsilon()  ) {
        return equalsRelative( lhs.x, rhs.x, tolerance ) && equalsRelative( lhs.y, rhs.y ) && equalsRelative( lhs.z, rhs.z );
    }

    class AbstractLocalSpace
    {
    public:
        virtual ~AbstractLocalSpace() { /* Nothing to do. */ }
        

        // accessors (get and set) for side, up, forward and position
        virtual Vec3 side (void) const = 0;
        virtual Vec3 setSide (Vec3 s) = 0;
        virtual Vec3 up (void) const = 0;
        virtual Vec3 setUp (Vec3 u) = 0;
        virtual Vec3 forward (void) const = 0;
        virtual Vec3 setForward (Vec3 f) = 0;
        virtual Vec3 position (void) const = 0;
        virtual Vec3 setPosition (Vec3 p) = 0;

        // use right-(or left-)handed coordinate space
        virtual bool rightHanded (void) const = 0;

        // reset transform to identity
        virtual void resetLocalSpace (void) = 0;

        // transform a direction in global space to its equivalent in local space
        virtual Vec3 localizeDirection (const Vec3& globalDirection) const = 0;

        // transform a point in global space to its equivalent in local space
        virtual Vec3 localizePosition (const Vec3& globalPosition) const = 0;

        // transform a point in local space to its equivalent in global space
        virtual Vec3 globalizePosition (const Vec3& localPosition) const = 0;

        // transform a direction in local space to its equivalent in global space
        virtual Vec3 globalizeDirection (const Vec3& localDirection) const = 0;

        // set "side" basis vector to normalized cross product of forward and up
        virtual void setUnitSideFromForwardAndUp (void) = 0;

        // regenerate the orthonormal basis vectors given a new forward
        // (which is expected to have unit length)
        virtual void regenerateOrthonormalBasisUF (const Vec3& newUnitForward) = 0;

        // for when the new forward is NOT of unit length
        virtual void regenerateOrthonormalBasis (const Vec3& newForward) = 0;

        // for supplying both a new forward and and new up
        virtual void regenerateOrthonormalBasis (const Vec3& newForward,
                                                 const Vec3& newUp) = 0;

        // rotate 90 degrees in the direction implied by rightHanded()
        virtual Vec3 localRotateForwardToSide (const Vec3& v) const = 0;
        virtual Vec3 globalRotateForwardToSide (const Vec3& globalForward) const=0;
    };


    // ----------------------------------------------------------------------------
    // LocalSpaceMixin is a mixin layer, a class template with a paramterized base
    // class.  Allows "LocalSpace-ness" to be layered on any class.


    template <class Super>
    class LocalSpaceMixin : public Super
    {
        // transformation as three orthonormal unit basis vectors and the
        // origin of the local space.  These correspond to the "rows" of
        // a 3x4 transformation matrix with [0 0 0 1] as the final column

    private:

        Vec3 _side;     //    side-pointing unit basis vector
        Vec3 _up;       //  upward-pointing unit basis vector
        Vec3 _forward;  // forward-pointing unit basis vector
        Vec3 _position; // origin of local space

    public:

        // accessors (get and set) for side, up, forward and position
        Vec3 side     (void) const {return _side;};
        Vec3 up       (void) const {return _up;};
        Vec3 forward  (void) const {return _forward;};
        Vec3 position (void) const {return _position;};
        Vec3 setSide     (Vec3 s) {return _side = s;};
        Vec3 setUp       (Vec3 u) {return _up = u;};
        Vec3 setForward  (Vec3 f) {return _forward = f;};
        Vec3 setPosition (Vec3 p) {return _position = p;};
        Vec3 setSide     (float x, float y, float z){return _side.set    (x,y,z);};
        Vec3 setUp       (float x, float y, float z){return _up.set      (x,y,z);};
        Vec3 setForward  (float x, float y, float z){return _forward.set (x,y,z);};
        Vec3 setPosition (float x, float y, float z){return _position.set(x,y,z);};


        // ------------------------------------------------------------------------
        // Global compile-time switch to control handedness/chirality: should
        // LocalSpace use a left- or right-handed coordinate system?  This can be
        // overloaded in derived types (e.g. vehicles) to change handedness.

        bool rightHanded (void) const {return true;}


        // ------------------------------------------------------------------------
        // constructors


        LocalSpaceMixin (void)
        {
            resetLocalSpace ();
        };

        LocalSpaceMixin (const Vec3& Side,
                         const Vec3& Up,
                         const Vec3& Forward,
                         const Vec3& Position)
            : _side( Side ), _up( Up ), _forward( Forward ), _position( Position ) {}


        LocalSpaceMixin (const Vec3& Up,
                         const Vec3& Forward,
                         const Vec3& Position)
            : _side(), _up( Up ), _forward( Forward ), _position( Position )
        {
            setUnitSideFromForwardAndUp ();
        }

        
        virtual ~LocalSpaceMixin() { /* Nothing to do. */ }
        

        // ------------------------------------------------------------------------
        // reset transform: set local space to its identity state, equivalent to a
        // 4x4 homogeneous transform like this:
        //
        //     [ X 0 0 0 ]
        //     [ 0 1 0 0 ]
        //     [ 0 0 1 0 ]
        //     [ 0 0 0 1 ]
        //
        // where X is 1 for a left-handed system and -1 for a right-handed system.

        void resetLocalSpace (void)
        {
            _forward.set (0, 0, 1);
            _side = localRotateForwardToSide (_forward);
            _up.set (0, 1, 0);
            _position.set (0, 0, 0);
        };


        // ------------------------------------------------------------------------
        // transform a direction in global space to its equivalent in local space


        Vec3 localizeDirection (const Vec3& globalDirection) const
        {
            // dot offset with local basis vectors to obtain local coordiantes
            return Vec3 (globalDirection.dot (_side),
                         globalDirection.dot (_up),
                         globalDirection.dot (_forward));
        };


        // ------------------------------------------------------------------------
        // transform a point in global space to its equivalent in local space


        Vec3 localizePosition (const Vec3& globalPosition) const
        {
            // global offset from local origin
            Vec3 globalOffset = globalPosition - _position;

            // dot offset with local basis vectors to obtain local coordiantes
            return localizeDirection (globalOffset);
        };


        // ------------------------------------------------------------------------
        // transform a point in local space to its equivalent in global space


        Vec3 globalizePosition (const Vec3& localPosition) const
        {
            return _position + globalizeDirection (localPosition);
        };


        // ------------------------------------------------------------------------
        // transform a direction in local space to its equivalent in global space


        Vec3 globalizeDirection (const Vec3& localDirection) const
        {
            return ((_side    * localDirection.x) +
                    (_up      * localDirection.y) +
                    (_forward * localDirection.z));
        };


        // ------------------------------------------------------------------------
        // set "side" basis vector to normalized cross product of forward and up


        void setUnitSideFromForwardAndUp (void)
        {
            // derive new unit side basis vector from forward and up
            if (rightHanded())
                _side.cross (_forward, _up);
            else
                _side.cross (_up, _forward);
            _side = _side.normalize ();
        }


        // ------------------------------------------------------------------------
        // regenerate the orthonormal basis vectors given a new forward
        // (which is expected to have unit length)


        void regenerateOrthonormalBasisUF (const Vec3& newUnitForward)
        {
            _forward = newUnitForward;

            // derive new side basis vector from NEW forward and OLD up
            setUnitSideFromForwardAndUp ();

            // derive new Up basis vector from new Side and new Forward
            // (should have unit length since Side and Forward are
            // perpendicular and unit length)
            if (rightHanded())
                _up.cross (_side, _forward);
            else
                _up.cross (_forward, _side);
        }


        // for when the new forward is NOT know to have unit length

        void regenerateOrthonormalBasis (const Vec3& newForward)
        {
            regenerateOrthonormalBasisUF (newForward.normalize());
        }


        // for supplying both a new forward and and new up

        void regenerateOrthonormalBasis (const Vec3& newForward,
                                         const Vec3& newUp)
        {
            _up = newUp;
            regenerateOrthonormalBasis (newForward.normalize());
        }


        // ------------------------------------------------------------------------
        // rotate, in the canonical direction, a vector pointing in the
        // "forward" (+Z) direction to the "side" (+/-X) direction


        Vec3 localRotateForwardToSide (const Vec3& v) const
        {
            return Vec3 (rightHanded () ? -v.z : +v.z,
                         v.y,
                         v.x);
        }

        // not currently used, just added for completeness

        Vec3 globalRotateForwardToSide (const Vec3& globalForward) const
        {
            const Vec3 localForward = localizeDirection (globalForward);
            const Vec3 localSide = localRotateForwardToSide (localForward);
            return globalizeDirection (localSide);
        }
    };


    // ----------------------------------------------------------------------------
    // Concrete LocalSpace class, and a global constant for the identity transform


    typedef LocalSpaceMixin<AbstractLocalSpace> LocalSpace;

    const LocalSpace gGlobalSpace;
    
    extern bool enableAnnotation;
    extern bool drawPhaseActive;
    
    // graphical annotation: master on/off switch
    inline bool annotationIsOn (void) {return enableAnnotation;}
    inline void setAnnotationOn (void) {enableAnnotation = true;}
    inline void setAnnotationOff (void) {enableAnnotation = false;}
    inline bool toggleAnnotationState (void) {return (enableAnnotation = !enableAnnotation);}
    
    template <class Super>
    class AnnotationMixin : public Super
    {
    public:
        
        // constructor
        AnnotationMixin ();
        
        // destructor
        virtual ~AnnotationMixin ();
        
        // ------------------------------------------------------------------------
        // trails / streamers
        //
        // these routines support visualization of a vehicle's recent path
        //
        // XXX conceivable trail/streamer should be a separate class,
        // XXX Annotation would "has-a" one (or more))
        
        // record a position for the current time, called once per update
        void recordTrailVertex (const float currentTime, const Vec3& position);
        
        // draw the trail as a dotted line, fading away with age
        void drawTrail (void) {drawTrail (grayColor (0.7f), gWhite);}
        void drawTrail  (const Color& trailColor, const Color& tickColor);
        
        // set trail parameters: the amount of time it represents and the
        // number of samples along its length.  re-allocates internal buffers.
        void setTrailParameters (const float duration, const int vertexCount);
        
        // forget trail history: used to prevent long streaks due to teleportation
        void clearTrailHistory (void);
        
        // ------------------------------------------------------------------------
        // drawing of lines, circles and (filled) disks to annotate steering
        // behaviors.  When called during App's simulation update phase,
        // these functions call a "deferred draw" routine which buffer the
        // arguments for use during the redraw phase.
        //
        // note: "circle" means unfilled
        //       "disk" means filled
        //       "XZ" means on a plane parallel to the X and Z axes (perp to Y)
        //       "3d" means the circle is perpendicular to the given "axis"
        //       "segments" is the number of line segments used to draw the circle
        
        // draw an opaque colored line segment between two locations in space
        void annotationLine (const Vec3& startPoint,
                             const Vec3& endPoint,
                             const Color& color) const;
        
        // draw a circle on the XZ plane
        void annotationXZCircle (const float radius,
                                 const Vec3& center,
                                 const Color& color,
                                 const int segments) const
        {
            annotationXZCircleOrDisk (radius, center, color, segments, false);
        }
        
        
        // draw a disk on the XZ plane
        void annotationXZDisk (const float radius,
                               const Vec3& center,
                               const Color& color,
                               const int segments) const
        {
            annotationXZCircleOrDisk (radius, center, color, segments, true);
        }
        
        
        // draw a circle perpendicular to the given axis
        void annotation3dCircle (const float radius,
                                 const Vec3& center,
                                 const Vec3& axis,
                                 const Color& color,
                                 const int segments) const
        {
            annotation3dCircleOrDisk (radius, center, axis, color, segments, false);
        }
        
        
        // draw a disk perpendicular to the given axis
        void annotation3dDisk (const float radius,
                               const Vec3& center,
                               const Vec3& axis,
                               const Color& color,
                               const int segments) const
        {
            annotation3dCircleOrDisk (radius, center, axis, color, segments, true);
        }
        
        //
        
        // ------------------------------------------------------------------------
        // support for annotation circles
        
        void annotationXZCircleOrDisk (const float radius,
                                       const Vec3& center,
                                       const Color& color,
                                       const int segments,
                                       const bool filled) const
        {
            annotationCircleOrDisk (radius,
                                    Vec3::zero,
                                    center,
                                    color,
                                    segments,
                                    filled,
                                    false); // "not in3d" -> on XZ plane
        }
        
        
        void annotation3dCircleOrDisk (const float radius,
                                       const Vec3& center,
                                       const Vec3& axis,
                                       const Color& color,
                                       const int segments,
                                       const bool filled) const
        {
            annotationCircleOrDisk (radius,
                                    axis,
                                    center,
                                    color,
                                    segments,
                                    filled,
                                    true); // "in3d"
        }
        
        void annotationCircleOrDisk (const float radius,
                                     const Vec3& axis,
                                     const Vec3& center,
                                     const Color& color,
                                     const int segments,
                                     const bool filled,
                                     const bool in3d) const;
        
        // ------------------------------------------------------------------------
    private:
        
        // trails
        int trailVertexCount;       // number of vertices in array (ring buffer)
        int trailIndex;             // array index of most recently recorded point
        float trailDuration;        // duration (in seconds) of entire trail
        float trailSampleInterval;  // desired interval between taking samples
        float trailLastSampleTime;  // global time when lat sample was taken
        int trailDottedPhase;       // dotted line: draw segment or not
        Vec3 curPosition;           // last reported position of vehicle
        Vec3* trailVertices;        // array (ring) of recent points along trail
        char* trailFlags;           // array (ring) of flag bits for trail points
    };
    
    
    class AbstractVehicle : public AbstractLocalSpace
    {
    public:
        virtual ~AbstractVehicle() { /* Nothing to do. */ }
        
        // mass (defaults to unity so acceleration=force)
        virtual float mass (void) const = 0;
        virtual float setMass (float) = 0;

        // size of bounding sphere, for obstacle avoidance, etc.
        virtual float radius (void) const = 0;
        virtual float setRadius (float) = 0;

        // velocity of vehicle
        virtual Vec3 velocity (void) const = 0;

        // speed of vehicle  (may be faster than taking magnitude of velocity)
        virtual float speed (void) const = 0;
        virtual float setSpeed (float) = 0;

        // groups of (pointers to) abstract vehicles, and iterators over them
        typedef std::vector<AbstractVehicle*> group;
        typedef group::const_iterator iterator;    

        // predict position of this vehicle at some time in the future
        // (assumes velocity remains constant)
        virtual Vec3 predictFuturePosition (const float predictionTime) const = 0;

        // ----------------------------------------------------------------------
        // XXX this vehicle-model-specific functionality functionality seems out
        // XXX of place on the abstract base class, but for now it is expedient

        // the maximum steering force this vehicle can apply
        virtual float maxForce (void) const = 0;
        virtual float setMaxForce (float) = 0;

        // the maximum speed this vehicle is allowed to move
        virtual float maxSpeed (void) const = 0;
        virtual float setMaxSpeed (float) = 0;

        // dp - added to support heterogeneous flocks
        virtual void update(const float currentTime, const float elapsedTime) = 0;
    };
    
    // more convenient short names for AbstractVehicle group and iterator
    typedef AbstractVehicle::group AVGroup;
    typedef AbstractVehicle::iterator AVIterator;
    // ----------------------------------------------------------------------------
    
    // ----------------------------------------------------------------------------
    
    /**
     * Pure virtual base class representing an abstract pathway in space.
     * Could be used for example in path following.
     */
    class Pathway {
    public:
        virtual ~Pathway() = 0;
        
        /**
         * Returns @c true if the path is valid, @c false otherwise.
         */
        virtual bool isValid() const = 0;
        
        /**
         * Given an arbitrary point ("A"), returns the nearest point ("P") on
         * this path center line.  Also returns, via output arguments, the path
         * tangent at P and a measure of how far A is outside the Pathway's
         * "tube".  Note that a negative distance indicates A is inside the
         * Pathway.
         *
         * If @c isValid is @c false the behavior is undefined.
         */
        virtual Vec3 mapPointToPath (const Vec3& point,
                                     Vec3& tangent,
                                     float& outside) const = 0;
        
        /**
         * Given a distance along the path, convert it to a point on the path.
         * If @c isValid is @c false the behavior is undefined.
         */
        virtual Vec3 mapPathDistanceToPoint (float pathDistance) const = 0;
        
        /**
         * Given an arbitrary point, convert it to a distance along the path.
         * If @c isValid is @c false the behavior is undefined.
         */
        virtual float mapPointToPathDistance (const Vec3& point) const = 0;
        
        /**
         * Returns @c true f the path is closed, otherwise @c false.
         */
        virtual bool isCyclic() const = 0;
        
        /**
         * Returns the length of the path.
         */
        virtual float length() const = 0;
        
    protected:
        /**
         * Protected to disable assigning instances of different inherited
         * classes to each other.
         *
         * @todo Should this be added or not? Have to read a bit...
         */
        // Pathway& operator=( Pathway const& );
        
    }; // class Pathway
    
    // ----------------------------------------------------------------------------
    // AbstractObstacle: a pure virtual base class for an abstract shape in
    // space, to be used with obstacle avoidance.  (Oops, its not "pure" since
    // I added a concrete method to PathIntersection 11-04-04 -cwr).
    
    
    class AbstractObstacle
    {
    public:
        
        virtual ~AbstractObstacle() { /* Nothing to do. */ }
        
        
        // compute steering for a vehicle to avoid this obstacle, if needed
        virtual Vec3 steerToAvoid (const AbstractVehicle& v,
                                   const float minTimeToCollision) const = 0;
        
        // PathIntersection object: used internally to analyze and store
        // information about intersections of vehicle paths and obstacles.
        class PathIntersection
        {
        public:
            bool intersect; // was an intersection found?
            float distance; // how far was intersection point from vehicle?
            Vec3 surfacePoint; // position of intersection
            Vec3 surfaceNormal; // unit normal at point of intersection
            Vec3 steerHint; // where to steer away from intersection
            bool vehicleOutside; // is the vehicle outside the obstacle?
            const AbstractObstacle* obstacle; // obstacle the path intersects
            
            // determine steering based on path intersection tests
            Vec3 steerToAvoidIfNeeded (const AbstractVehicle& vehicle,
                                       const float minTimeToCollision) const;
            
        };
        
        // find first intersection of a vehicle's path with this obstacle
        // (this must be specialized for each new obstacle shape class)
        virtual void
        findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                         PathIntersection& pi)
        const
        = 0 ;
        
        // virtual function for drawing -- normally does nothing, can be
        // specialized by derived types to provide graphics for obstacles
        virtual void draw (const bool filled,
                           const Color& color,
                           const Vec3& viewpoint)
        const
        = 0 ;
        
        // seenFrom (eversion): does this obstacle contrain vehicle to stay
        // inside it or outside it (or both)?  "Inside" describes a clear space
        // within a solid (for example, the interior of a room inside its
        // walls). "Ouitside" describes a solid chunk in the midst of clear
        // space.
        enum seenFromState {outside, inside, both};
        virtual seenFromState seenFrom (void) const = 0;
        virtual void setSeenFrom (seenFromState s) = 0;
    };
    
    
    // an STL vector of AbstractObstacle pointers and an iterator for it:
    typedef std::vector<AbstractObstacle*> ObstacleGroup;
    typedef ObstacleGroup::const_iterator ObstacleIterator;
    
    
    // ----------------------------------------------------------------------------
    // Obstacle is a utility base class providing some shared functionality
    
    
    class Obstacle : public AbstractObstacle
    {
    public:
        
        Obstacle (void) : _seenFrom (outside) {}
        
        virtual ~Obstacle() { /* Nothing to do. */ }
        
        // compute steering for a vehicle to avoid this obstacle, if needed
        Vec3 steerToAvoid (const AbstractVehicle& v,
                           const float minTimeToCollision)
        const;
        
        // static method to apply steerToAvoid to nearest obstacle in an
        // ObstacleGroup
        static Vec3 steerToAvoidObstacles (const AbstractVehicle& vehicle,
                                           const float minTimeToCollision,
                                           const ObstacleGroup& obstacles);
        
        // static method to find first vehicle path intersection in an
        // ObstacleGroup
        static void
        firstPathIntersectionWithObstacleGroup (const AbstractVehicle& vehicle,
                                                const ObstacleGroup& obstacles,
                                                PathIntersection& nearest,
                                                PathIntersection& next);
        
        // default do-nothing draw function (derived class can overload this)
        void draw (const bool, const Color&, const Vec3&) const {}
        
        seenFromState seenFrom (void) const {return _seenFrom;}
        void setSeenFrom (seenFromState s) {_seenFrom = s;}
    private:
        seenFromState _seenFrom;
    };
    
    
    // ----------------------------------------------------------------------------
    // SphereObstacle a simple ball-shaped obstacle
    
    
    class SphereObstacle : public Obstacle
    {
    public:
        float radius;
        Vec3 center;
        
        // constructors
        SphereObstacle (float r, Vec3 c) : radius(r), center (c) {}
        SphereObstacle (void) : radius(1), center (Vec3::zero) {}
        
        virtual ~SphereObstacle() { /* Nothing to do. */ }
        
        // find first intersection of a vehicle's path with this obstacle
        void findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                              PathIntersection& pi)
        const;
    };
    
    
    // ----------------------------------------------------------------------------
    // LocalSpaceObstacle: a mixture of LocalSpace and Obstacle methods
    
    
    typedef LocalSpaceMixin<Obstacle> LocalSpaceObstacle;
    
    
    // ----------------------------------------------------------------------------
    // BoxObstacle: a box-shaped (cuboid) obstacle of a given height, width,
    // depth, position and orientation.  The box is centered on and aligned
    // with a local space.
    
    
    class BoxObstacle : public LocalSpaceObstacle
    {
    public:
        float width;  // width  of box centered on local X (side)    axis
        float height; // height of box centered on local Y (up)      axis
        float depth;  // depth  of box centered on local Z (forward) axis
        
        // constructors
        BoxObstacle (float w, float h, float d) : width(w), height(h), depth(d) {}
        BoxObstacle (void) :  width(1.0f), height(1.0f), depth(1.0f) {}
        
        virtual ~BoxObstacle() { /* Nothing to do. */ }
        
        
        // find first intersection of a vehicle's path with this obstacle
        void findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                              PathIntersection& pi)
        const;
    };
    
    
    // ----------------------------------------------------------------------------
    // PlaneObstacle: a planar obstacle of a given position and orientation.
    // The plane is defined as the XY (aka side/up) plane of a local space.
    // The +Z (forward) half-space is considered "outside" the obstacle.
    //
    // This is also the base class for several other obstacles which represent
    // 2d shapes (rectangle, triangle, ...) arbitarily oriented and positioned
    // in 2d space.  They specialize this class via xyPointInsideShape which
    // tests if a given point on the XZ plane is inside the obstacle's shape.
    
    
    class PlaneObstacle : public LocalSpaceObstacle
    {
    public:
        // constructors
        PlaneObstacle (void) {}
        PlaneObstacle (const Vec3& s,
                       const Vec3& u,
                       const Vec3& f,
                       const Vec3& p)
        : LocalSpaceObstacle( s, u, f, p )
        {
            /*
             setSide (s);
             setUp (u);
             setForward (f);
             setPosition (p);
             */
        }
        
        // find first intersection of a vehicle's path with this obstacle
        void findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                              PathIntersection& pi)
        const;
        
        // determines if a given point on XY plane is inside obstacle shape
        virtual bool xyPointInsideShape (const Vec3& /*point*/,
                                         float /*radius*/) const
        {
            return true; // always true for PlaneObstacle
        }
    };
    
    
    // ----------------------------------------------------------------------------
    // RectangleObstacle: a rectangular obstacle of a given height, width,
    // position and orientation.  It is a rectangle centered on the XY (aka
    // side/up) plane of a local space.
    
    
    class RectangleObstacle : public PlaneObstacle
    {
    public:
        float width;  // width  of rectangle centered on local X (side) axis
        float height; // height of rectangle centered on local Y (up)   axis
        
        // constructors
        RectangleObstacle (float w, float h) : width(w), height(h) {}
        RectangleObstacle (void) :  width(1.0f), height(1.0f) {}
        RectangleObstacle (float w, float h, const Vec3& s,
                           const Vec3& u, const Vec3& f, const Vec3& p,
                           seenFromState sf)
        : PlaneObstacle( s, u, f, p ), width(w), height(h)
        {
            /*
             setSide (s);
             setUp (u);
             setForward (f);
             setPosition (p);
             */
            setSeenFrom (sf);
        }
        
        virtual ~RectangleObstacle() { /* Nothing to do. */ }
        
        // determines if a given point on XY plane is inside obstacle shape
        bool xyPointInsideShape (const Vec3& point, float radius) const;
    };
    
    template <class Super>
    class SteerLibraryMixin : public Super
    {
    public:
        using Super::velocity;
        using Super::maxSpeed;
        using Super::speed;
        using Super::radius;
        using Super::maxForce;
        using Super::forward;
        using Super::position;
        using Super::side;
        using Super::up;
        using Super::predictFuturePosition;
        
    public:
        
        // Constructor: initializes state
        SteerLibraryMixin ()
        {
            // set inital state
            reset ();
        }
        
        // reset state
        void reset (void)
        {
            // initial state of wander behavior
            WanderSide = 0;
            WanderUp = 0;
            
            // default to non-gaudyPursuitAnnotation
            gaudyPursuitAnnotation = false;
        }
        
        // -------------------------------------------------- steering behaviors
        
        // Wander behavior
        float WanderSide;
        float WanderUp;
        Vec3 steerForWander (float dt);
        
        // Seek behavior
        Vec3 steerForSeek (const Vec3& target);
        
        // Flee behavior
        Vec3 steerForFlee (const Vec3& target);
        
        // xxx proposed, experimental new seek/flee [cwr 9-16-02]
        Vec3 xxxsteerForFlee (const Vec3& target);
        Vec3 xxxsteerForSeek (const Vec3& target);
        
        // Path Following behaviors
        Vec3 steerToFollowPath (const int direction,
                                const float predictionTime,
                                Pathway& path);
        Vec3 steerToStayOnPath (const float predictionTime, Pathway& path);
        
        // ------------------------------------------------------------------------
        // Obstacle Avoidance behavior
        //
        // Returns a steering force to avoid a given obstacle.  The purely
        // lateral steering force will turn our vehicle towards a silhouette edge
        // of the obstacle.  Avoidance is required when (1) the obstacle
        // intersects the vehicle's current path, (2) it is in front of the
        // vehicle, and (3) is within minTimeToCollision seconds of travel at the
        // vehicle's current velocity.  Returns a zero vector value (Vec3::zero)
        // when no avoidance is required.
        
        
        Vec3 steerToAvoidObstacle (const float minTimeToCollision,
                                   const Obstacle& obstacle);
        
        
        // avoids all obstacles in an ObstacleGroup
        
        Vec3 steerToAvoidObstacles (const float minTimeToCollision,
                                    const ObstacleGroup& obstacles);
        
        
        // ------------------------------------------------------------------------
        // Unaligned collision avoidance behavior: avoid colliding with other
        // nearby vehicles moving in unconstrained directions.  Determine which
        // (if any) other other vehicle we would collide with first, then steers
        // to avoid the site of that potential collision.  Returns a steering
        // force vector, which is zero length if there is no impending collision.
        
        
        Vec3 steerToAvoidNeighbors (const float minTimeToCollision,
                                    const AVGroup& others);
        
        
        // Given two vehicles, based on their current positions and velocities,
        // determine the time until nearest approach
        float predictNearestApproachTime (AbstractVehicle& otherVehicle);
        
        // Given the time until nearest approach (predictNearestApproachTime)
        // determine position of each vehicle at that time, and the distance
        // between them
        float computeNearestApproachPositions (AbstractVehicle& otherVehicle,
                                               float time);
        
        
        /// XXX globals only for the sake of graphical annotation
        Vec3 hisPositionAtNearestApproach;
        Vec3 ourPositionAtNearestApproach;
        
        
        // ------------------------------------------------------------------------
        // avoidance of "close neighbors" -- used only by steerToAvoidNeighbors
        //
        // XXX  Does a hard steer away from any other agent who comes withing a
        // XXX  critical distance.  Ideally this should be replaced with a call
        // XXX  to steerForSeparation.
        
        
        Vec3 steerToAvoidCloseNeighbors (const float minSeparationDistance,
                                         const AVGroup& others);
        
        
        // ------------------------------------------------------------------------
        // used by boid behaviors
        
        
        bool inBoidNeighborhood (const AbstractVehicle& otherVehicle,
                                 const float minDistance,
                                 const float maxDistance,
                                 const float cosMaxAngle);
        
        
        // ------------------------------------------------------------------------
        // Separation behavior -- determines the direction away from nearby boids
        
        
        Vec3 steerForSeparation (const float maxDistance,
                                 const float cosMaxAngle,
                                 const AVGroup& flock);
        
        
        // ------------------------------------------------------------------------
        // Alignment behavior
        
        Vec3 steerForAlignment (const float maxDistance,
                                const float cosMaxAngle,
                                const AVGroup& flock);
        
        
        // ------------------------------------------------------------------------
        // Cohesion behavior
        
        
        Vec3 steerForCohesion (const float maxDistance,
                               const float cosMaxAngle,
                               const AVGroup& flock);
        
        
        // ------------------------------------------------------------------------
        // pursuit of another vehicle (& version with ceiling on prediction time)
        
        
        Vec3 steerForPursuit (const AbstractVehicle& quarry);
        
        Vec3 steerForPursuit (const AbstractVehicle& quarry,
                              const float maxPredictionTime);
        
        // for annotation
        bool gaudyPursuitAnnotation;
        
        
        // ------------------------------------------------------------------------
        // evasion of another vehicle
        
        
        Vec3 steerForEvasion (const AbstractVehicle& menace,
                              const float maxPredictionTime);
        
        
        // ------------------------------------------------------------------------
        // tries to maintain a given speed, returns a maxForce-clipped steering
        // force along the forward/backward axis
        
        
        Vec3 steerForTargetSpeed (const float targetSpeed);
        
        
        // ----------------------------------------------------------- utilities
        // XXX these belong somewhere besides the steering library
        // XXX above AbstractVehicle, below SimpleVehicle
        // XXX ("utility vehicle"?)
        
        // xxx cwr experimental 9-9-02 -- names OK?
        bool isAhead (const Vec3& target) const {return isAhead (target, 0.707f);};
        bool isAside (const Vec3& target) const {return isAside (target, 0.707f);};
        bool isBehind (const Vec3& target) const {return isBehind (target, -0.707f);};
        
        bool isAhead (const Vec3& target, float cosThreshold) const
        {
            const Vec3 targetDirection = (target - position ()).normalize ();
            return forward().dot(targetDirection) > cosThreshold;
        };
        bool isAside (const Vec3& target, float cosThreshold) const
        {
            const Vec3 targetDirection = (target - position ()).normalize ();
            const float dp = forward().dot(targetDirection);
            return (dp < cosThreshold) && (dp > -cosThreshold);
        };
        bool isBehind (const Vec3& target, float cosThreshold) const
        {
            const Vec3 targetDirection = (target - position()).normalize ();
            return forward().dot(targetDirection) < cosThreshold;
        };
        
        
        // ------------------------------------------------ graphical annotation
        // (parameter names commented out to prevent compiler warning from "-W")
        
        
        // called when steerToAvoidObstacles decides steering is required
        // (default action is to do nothing, layered classes can overload it)
        virtual void annotateAvoidObstacle (const float /*minDistanceToCollision*/)
        {
        }
        
        // called when steerToFollowPath decides steering is required
        // (default action is to do nothing, layered classes can overload it)
        virtual void annotatePathFollowing (const Vec3& /*future*/,
                                            const Vec3& /*onPath*/,
                                            const Vec3& /*target*/,
                                            const float /*outside*/)
        {
        }
        
        // called when steerToAvoidCloseNeighbors decides steering is required
        // (default action is to do nothing, layered classes can overload it)
        virtual void annotateAvoidCloseNeighbor (const AbstractVehicle& /*other*/,
                                                 const float /*additionalDistance*/)
        {
        }
        
        // called when steerToAvoidNeighbors decides steering is required
        // (default action is to do nothing, layered classes can overload it)
        virtual void annotateAvoidNeighbor (const AbstractVehicle& /*threat*/,
                                            const float /*steer*/,
                                            const Vec3& /*ourFuture*/,
                                            const Vec3& /*threatFuture*/)
        {
        }
    };


    


    // SimpleVehicle_1 adds concrete LocalSpace methods to AbstractVehicle
    typedef LocalSpaceMixin<AbstractVehicle> SimpleVehicle_1;


    // SimpleVehicle_2 adds concrete annotation methods to SimpleVehicle_1
    typedef AnnotationMixin<SimpleVehicle_1> SimpleVehicle_2;


    // SimpleVehicle_3 adds concrete steering methods to SimpleVehicle_2
    typedef SteerLibraryMixin<SimpleVehicle_2> SimpleVehicle_3;


    // SimpleVehicle adds concrete vehicle methods to SimpleVehicle_3
    class SimpleVehicle : public SimpleVehicle_3
    {
    public:

        // constructor
        SimpleVehicle ();

        // destructor
        ~SimpleVehicle ();

        // reset vehicle state
        void reset (void)
        {
            // reset LocalSpace state
            resetLocalSpace ();

            // reset SteerLibraryMixin state
            // (XXX this seems really fragile, needs to be redesigned XXX)
            SimpleVehicle_3::reset ();

            setMass (1);          // mass (defaults to 1 so acceleration=force)
            setSpeed (0);         // speed along Forward direction.

            setRadius (0.5f);     // size of bounding sphere

            setMaxForce (0.1f);   // steering force is clipped to this magnitude
            setMaxSpeed (1.0f);   // velocity is clipped to this magnitude

            // reset bookkeeping to do running averages of these quanities
            resetSmoothedPosition ();
            resetSmoothedCurvature ();
            resetSmoothedAcceleration ();
        }

        // get/set mass
        float mass (void) const {return _mass;}
        float setMass (float m) {return _mass = m;}

        // get velocity of vehicle
        Vec3 velocity (void) const {return forward() * _speed;}

        // get/set speed of vehicle  (may be faster than taking mag of velocity)
        float speed (void) const {return _speed;}
        float setSpeed (float s) {return _speed = s;}

        // size of bounding sphere, for obstacle avoidance, etc.
        float radius (void) const {return _radius;}
        float setRadius (float m) {return _radius = m;}

        // get/set maxForce
        float maxForce (void) const {return _maxForce;}
        float setMaxForce (float mf) {return _maxForce = mf;}

        // get/set maxSpeed
        float maxSpeed (void) const {return _maxSpeed;}
        float setMaxSpeed (float ms) {return _maxSpeed = ms;}

        // ratio of speed to max possible speed (0 slowest, 1 fastest)
        float relativeSpeed (void) const {return speed () / maxSpeed ();}


        // apply a given steering force to our momentum,
        // adjusting our orientation to maintain velocity-alignment.
        void applySteeringForce (const Vec3& force, const float deltaTime);

        // the default version: keep FORWARD parallel to velocity, change
        // UP as little as possible.
        virtual void regenerateLocalSpace (const Vec3& newVelocity,
                                           const float elapsedTime);

        // alternate version: keep FORWARD parallel to velocity, adjust UP
        // according to a no-basis-in-reality "banking" behavior, something
        // like what birds and airplanes do.  (XXX experimental cwr 6-5-03)
        void regenerateLocalSpaceForBanking (const Vec3& newVelocity,
                                             const float elapsedTime);

        // adjust the steering force passed to applySteeringForce.
        // allows a specific vehicle class to redefine this adjustment.
        // default is to disallow backward-facing steering at low speed.
        // xxx experimental 8-20-02
        virtual Vec3 adjustRawSteeringForce (const Vec3& force,
                                             const float deltaTime);

        // apply a given braking force (for a given dt) to our momentum.
        // xxx experimental 9-6-02
        void applyBrakingForce (const float rate, const float deltaTime);

        // predict position of this vehicle at some time in the future
        // (assumes velocity remains constant)
        Vec3 predictFuturePosition (const float predictionTime) const;

        // get instantaneous curvature (since last update)
        float curvature (void) const {return _curvature;}

        // get/reset smoothedCurvature, smoothedAcceleration and smoothedPosition
        float smoothedCurvature (void) {return _smoothedCurvature;}
        float resetSmoothedCurvature (float value = 0)
        {
            _lastForward = Vec3::zero;
            _lastPosition = Vec3::zero;
            return _smoothedCurvature = _curvature = value;
        }
        Vec3 smoothedAcceleration (void) {return _smoothedAcceleration;}
        Vec3 resetSmoothedAcceleration (const Vec3& value = Vec3::zero)
        {
            return _smoothedAcceleration = value;
        }
        Vec3 smoothedPosition (void) {return _smoothedPosition;}
        Vec3 resetSmoothedPosition (const Vec3& value = Vec3::zero)
        {
            return _smoothedPosition = value;
        }

        // give each vehicle a unique number
        int serialNumber;
        static int serialNumberCounter;

        // draw lines from vehicle's position showing its velocity and acceleration
        void annotationVelocityAcceleration (float maxLengthA, float maxLengthV);
        void annotationVelocityAcceleration (float maxLength)
            {annotationVelocityAcceleration (maxLength, maxLength);}
        void annotationVelocityAcceleration (void)
            {annotationVelocityAcceleration (3, 3);}

        // set a random "2D" heading: set local Up to global Y, then effectively
        // rotate about it by a random angle (pick random forward, derive side).
        void randomizeHeadingOnXZPlane (void)
        {
            setUp (Vec3::up);
            setForward (RandomUnitVectorOnXZPlane ());
            setSide (localRotateForwardToSide (forward()));
        }

    private:

        float _mass;       // mass (defaults to unity so acceleration=force)

        float _radius;     // size of bounding sphere, for obstacle avoidance, etc.

        float _speed;      // speed along Forward direction.  Because local space
                           // is velocity-aligned, velocity = Forward * Speed

        float _maxForce;   // the maximum steering force this vehicle can apply
                           // (steering force is clipped to this magnitude)

        float _maxSpeed;   // the maximum speed this vehicle is allowed to move
                           // (velocity is clipped to this magnitude)

        float _curvature;
        Vec3 _lastForward;
        Vec3 _lastPosition;
        Vec3 _smoothedPosition;
        float _smoothedCurvature;
        Vec3 _smoothedAcceleration;

        // measure path curvature (1/turning-radius), maintain smoothed version
        void measurePathCurvature (const float elapsedTime);
    };

    class Camera : public LocalSpace
    {
    public:

        // constructor
        Camera ();
        virtual ~Camera() { /* Nothing to do? */ }

        // reset all camera state to default values
        void reset (void);

        // "look at" point, center of view
        Vec3 target;

        // vehicle being tracked
        const AbstractVehicle* vehicleToTrack;

        // aim at predicted position of vehicleToTrack, this far into thefuture
        float aimLeadTime;

        // per frame simulation update
        void update (const float currentTime,
                     const float elapsedTime,
                     const bool simulationPaused);
        void update (const float currentTime, const float elapsedTime)
        {update (currentTime, elapsedTime, false);};

        // helper function for "drag behind" mode
        Vec3 constDistHelper (const float elapsedTime);

        // Smoothly move camera ...
        void smoothCameraMove (const Vec3& newPosition,
                               const Vec3& newTarget,
                               const Vec3& newUp,
                               const float elapsedTime);

        void doNotSmoothNextMove (void) {smoothNextMove = false;};

        bool smoothNextMove;
        float smoothMoveSpeed;

        // adjust the offset vector of the current camera mode based on a
        // "mouse adjustment vector" from App (xxx experiment 10-17-02)
        void mouseAdjustOffset (const Vec3& adjustment);
        Vec3 mouseAdjust2 (const bool polar,
                           const Vec3& adjustment,
                           const Vec3& offsetToAdjust);
        Vec3 mouseAdjustPolar (const Vec3& adjustment,
                               const Vec3& offsetToAdjust)
        {return mouseAdjust2 (true, adjustment, offsetToAdjust);};
        Vec3 mouseAdjustOrtho (const Vec3& adjustment,
                               const Vec3& offsetToAdjust)
        {return mouseAdjust2 (false, adjustment, offsetToAdjust);};

        // xxx since currently (10-21-02) the camera's Forward and Side basis
        // xxx vectors are not being set, construct a temporary local space for
        // xxx the camera view -- so as not to make the camera behave
        // xxx differently (which is to say, correctly) during mouse adjustment.
        LocalSpace ls;
        const LocalSpace& xxxls (void)
        {ls.regenerateOrthonormalBasis (target - position(), up()); return ls;}


        // camera mode selection
        enum cameraMode 
            {
                // marks beginning of list
                cmStartMode,
            
                // fixed global position and aimpoint
                cmFixed,

                // camera position is directly above (in global Up/Y) target
                // camera up direction is target's forward direction
                cmStraightDown,

                // look at subject vehicle, adjusting camera position to be a
                // constant distance from the subject
                cmFixedDistanceOffset,

                // camera looks at subject vehicle from a fixed offset in the
                // local space of the vehicle (as if attached to the vehicle)
                cmFixedLocalOffset,

                // camera looks in the vehicle's forward direction, camera
                // position has a fixed local offset from the vehicle.
                cmOffsetPOV,

                // cmFixedPositionTracking // xxx maybe?

                // marks the end of the list for cycling (to cmStartMode+1)
                cmEndMode
            };

        // current mode for this camera instance
        cameraMode mode;

        // string naming current camera mode, used by App
        char const* modeName (void);

        // select next camera mode, used by App
        void selectNextMode (void);

        // the mode that comes after the given mode (used by selectNextMode)
        cameraMode successorMode (const cameraMode cm) const;

        // "static" camera mode parameters
        Vec3 fixedPosition;
        Vec3 fixedTarget;
        Vec3 fixedUp;

        // "constant distance from vehicle" camera mode parameters
        float fixedDistDistance;             // desired distance from it
        float fixedDistVOffset;              // fixed vertical offset from it

        // "look straight down at vehicle" camera mode parameters
        float lookdownDistance;             // fixed vertical offset from it

        // "fixed local offset" camera mode parameters
        Vec3 fixedLocalOffset;

        // "offset POV" camera mode parameters
        Vec3 povOffset;
    };

    class AbstractPlugIn
    {
    public:
        
        virtual ~AbstractPlugIn() { /* Nothing to do. */ }
        
        // generic PlugIn actions: open, update, redraw, close and reset
        virtual void open (void) = 0;
        virtual void update (const float currentTime, const float elapsedTime) = 0;
        virtual void redraw (const float currentTime, const float elapsedTime) = 0;
        virtual void close (void) = 0;
        virtual void reset (void) = 0;

        // return a pointer to this instance's character string name
        virtual const char* name (void) = 0;

        // numeric sort key used to establish user-visible PlugIn ordering
        // ("built ins" have keys greater than 0 and less than 1)
        virtual float selectionOrderSortKey (void) = 0;

        // allows a PlugIn to nominate itself as App's initially selected
        // (default) PlugIn, which is otherwise the first in "selection order"
        virtual bool requestInitialSelection (void) = 0;

        // handle function keys (which are reserved by SterTest for PlugIns)
        virtual void handleFunctionKeys (int keyNumber) = 0;

        // print "mini help" documenting function keys handled by this PlugIn
        virtual void printMiniHelpForFunctionKeys (void) = 0;

        // return an AVGroup (an STL vector of AbstractVehicle pointers) of
        // all vehicles(/agents/characters) defined by the PlugIn
        virtual const AVGroup& allVehicles (void) = 0;
    };


    class PlugIn : public AbstractPlugIn
    {
    public:
        // prototypes for function pointers used with PlugIns
        typedef void (* plugInCallBackFunction) (PlugIn& clientObject);
        typedef void (* voidCallBackFunction) (void);
        typedef void (* timestepCallBackFunction) (const float currentTime,
                                                   const float elapsedTime);

        // constructor
        PlugIn (void);

        // destructor
        virtual ~PlugIn();

        // default reset method is to do a close then an open
        void reset (void) {close (); open ();}

        // default sort key (after the "built ins")
        float selectionOrderSortKey (void) {return 1.0f;}

        // default is to NOT request to be initially selected
        bool requestInitialSelection (void) {return false;}

        // default function key handler: ignore all
        // (parameter names commented out to prevent compiler warning from "-W")
        void handleFunctionKeys (int /*keyNumber*/) {}

        // default "mini help": print nothing
        void printMiniHelpForFunctionKeys (void) {}

        // returns pointer to the next PlugIn in "selection order"
        PlugIn* next (void);

        // format instance to characters for printing to stream
        friend std::ostream& operator<< (std::ostream& os, PlugIn& pi)
        {
            os << "<PlugIn " << '"' << pi.name() << '"' << ">";
            return os;
        }

        // CLASS FUNCTIONS

        // search the class registry for a Plugin with the given name
        static PlugIn* findByName (const char* string);

        // apply a given function to all PlugIns in the class registry
        static void applyToAll (plugInCallBackFunction f);

        // sort PlugIn registry by "selection order"
        static void sortBySelectionOrder (void);

        // returns pointer to default PlugIn (currently, first in registry)
        static PlugIn* findDefault (void);

    private:

        // save this instance in the class's registry of instances
        void addToRegistry (void);

        // This array stores a list of all PlugIns.  It is manipulated by the
        // constructor and destructor, and used in findByName and applyToAll.
        static const int totalSizeOfRegistry;
        static int itemsInRegistry;
        static PlugIn* registry[];
    };

    // ----------------------------------------------------------------------------
    // appliaction related
    class App {
    public:
        // ------------------------------------------------------ component objects

        // clock keeps track of both "real time" and "simulation time"
        static Clock clock;

        // camera automatically tracks selected vehicle
        static Camera camera;

        // ------------------------------------------ addresses of selected objects

        // currently selected plug-in (user can choose or cycle through them)
        static PlugIn* selectedPlugIn;

        // currently selected vehicle.  Generally the one the camera follows and
        // for which additional information may be displayed.  Clicking the mouse
        // near a vehicle causes it to become the Selected Vehicle.
        static AbstractVehicle* selectedVehicle;

        // -------------------------------------------- initialize, update and exit

        // initialize App
        //     XXX  if I switch from "totally static" to "singleton"
        //     XXX  class structure this becomes the constructor
        static void initialize (void);

        // main update function: step simulation forward and redraw scene
        static void updateSimulationAndRedraw (void);

        // exit App with a given text message or error code
        static void errorExit (const char* message);
        static void exit (int exitCode);

        // ------------------------------------------------------- PlugIn interface

        // select the default PlugIn
        static void selectDefaultPlugIn (void);
        
        // select the "next" plug-in, cycling through "plug-in selection order"
        static void selectNextPlugIn (void);

        // handle function keys an a per-plug-in basis
        static void functionKeyForPlugIn (int keyNumber);

        // return name of currently selected plug-in
        static const char* nameOfSelectedPlugIn (void);

        // open the currently selected plug-in
        static void openSelectedPlugIn (void);

        // do a simulation update for the currently selected plug-in
        static void updateSelectedPlugIn (const float currentTime,
                                          const float elapsedTime);

        // redraw graphics for the currently selected plug-in
        static void redrawSelectedPlugIn (const float currentTime,
                                          const float elapsedTime);

        // close the currently selected plug-in
        static void closeSelectedPlugIn (void);

        // reset the currently selected plug-in
        static void resetSelectedPlugIn (void);

        static const AVGroup& allVehiclesOfSelectedPlugIn(void);

        // ---------------------------------------------------- App phase

        static bool phaseIsDraw     (void) {return phase == drawPhase;}
        static bool phaseIsUpdate   (void) {return phase == updatePhase;}
        static bool phaseIsOverhead (void) {return phase == overheadPhase;}

        static float phaseTimerDraw     (void) {return phaseTimers[drawPhase];}
        static float phaseTimerUpdate   (void) {return phaseTimers[updatePhase];}
        // XXX get around shortcomings in current implementation, see note
        // XXX in updateSimulationAndRedraw
        //static float phaseTimerOverhead(void){return phaseTimers[overheadPhase];}
        static float phaseTimerOverhead (void)
        {
            return (clock.getElapsedRealTime() -
                    (phaseTimerDraw() + phaseTimerUpdate()));
        }

        // ------------------------------------------------------ delayed reset XXX

        // XXX to be reconsidered
        static void queueDelayedResetPlugInXXX (void);
        static void doDelayedResetPlugInXXX (void);

        // ------------------------------------------------------ vehicle selection

        // select the "next" vehicle: cycle through the registry
        static void selectNextVehicle (void);

        // select vehicle nearest the given screen position (e.g.: of the mouse)
        static void selectVehicleNearestScreenPosition (int x, int y);

        // ---------------------------------------------------------- mouse support

        // Find the AbstractVehicle whose screen position is nearest the
        // current the mouse position.  Returns NULL if mouse is outside
        // this window or if there are no AbstractVehicles.
        static AbstractVehicle* vehicleNearestToMouse (void);

        // Find the AbstractVehicle whose screen position is nearest the
        // given window coordinates, typically the mouse position.  Note
        // this will return NULL if there are no AbstractVehicles.
        static AbstractVehicle* findVehicleNearestScreenPosition (int x, int y);

        // for storing most recent mouse state
        static int mouseX;
        static int mouseY;
        static bool mouseInWindow;

        // ------------------------------------------------------- camera utilities

        // set a certain initial camera state used by several plug-ins
        static void init2dCamera (AbstractVehicle& selected);
        static void init2dCamera (AbstractVehicle& selected,
                                  float distance,
                                  float elevation);
        static void init3dCamera (AbstractVehicle& selected);
        static void init3dCamera (AbstractVehicle& selected,
                                  float distance,
                                  float elevation);

        // set initial position of camera based on a vehicle
        static void position3dCamera (AbstractVehicle& selected);
        static void position3dCamera (AbstractVehicle& selected,
                                      float distance,
                                      float elevation);
        static void position2dCamera (AbstractVehicle& selected);
        static void position2dCamera (AbstractVehicle& selected,
                                      float distance,
                                      float elevation);

        // camera updating utility used by several (all?) plug-ins
        static void updateCamera (const float currentTime,
                                  const float elapsedTime,
                                  const AbstractVehicle& selected);

        // some camera-related default constants
        static const float camera2dElevation;
        static const float cameraTargetDistance;
        static const Vec3 cameraTargetOffset;

        // ------------------------------------------------ graphics and annotation

        // do all initialization related to graphics
        static void initializeGraphics (void);

        // ground plane grid-drawing utility used by several plug-ins
        static void gridUtility (const Vec3& gridTarget);

        // draws a gray disk on the XZ plane under a given vehicle
        static void highlightVehicleUtility (const AbstractVehicle& vehicle);

        // draws a gray circle on the XZ plane under a given vehicle
        static void circleHighlightVehicleUtility (const AbstractVehicle& vehicle);

        // draw a box around a vehicle aligned with its local space
        // xxx not used as of 11-20-02
        static void drawBoxHighlightOnVehicle (const AbstractVehicle& v,
                                               const Color& color);

        // draws a colored circle (perpendicular to view axis) around the center
        // of a given vehicle.  The circle's radius is the vehicle's radius times
        // radiusMultiplier.
        static void drawCircleHighlightOnVehicle (const AbstractVehicle& v,
                                                  const float radiusMultiplier,
                                                  const Color& color);

        // ----------------------------------------------------------- console text

        // print a line on the console with "App: " then the given ending
        static void printMessage (const char* message);
        static void printMessage (const std::ostringstream& message);

        // like printMessage but prefix is "App: Warning: "
        static void printWarning (const char* message);
        static void printWarning (const std::ostringstream& message);

        // print list of known commands
        static void keyboardMiniHelp (void);

        // ---------------------------------------------------------------- private

    private:
        static int phase;
        static int phaseStack[];
        static int phaseStackIndex;
        static float phaseTimers[];
        static float phaseTimerBase;
        static const int phaseStackSize;
        static void pushPhase (const int newPhase);
        static void popPhase (void);
        static void initPhaseTimers (void);
        static void updatePhaseTimers (void);

        // XXX apparently MS VC6 cannot handle initialized static const members,
        // XXX so they have to be initialized not-inline.
        // static const int drawPhase = 2;
        // static const int updatePhase = 1;
        // static const int overheadPhase = 0;
        static const int drawPhase;
        static const int updatePhase;
        static const int overheadPhase;
    };
    
    
    // ------------------------------------------------------------------------
    // warn when draw functions are called during OpenSteerDemo's update phase
    //
    // XXX perhaps this should be made to "melt away" when not in debug mode?
    
    void warnIfInUpdatePhase2( const char* name);
    
    // hosting application must provide this bool. It's true when updating and not drawing,
    // false otherwise.
    // it has been externed as a first step in making the Draw library useful from
    // other applications besides OpenSteerDemo
    extern bool updatePhaseActive;
    
    inline void warnIfInUpdatePhase (const char* name)
    {
        if (updatePhaseActive)
        {
            warnIfInUpdatePhase2 (name);
        }
    }
    
    // ----------------------------------------------------------------------------
    // this is a typedef for a triangle draw routine which can be passed in
    // when using rendering API's of the user's choice.
    typedef void (*drawTriangleRoutine) (const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Color& color);
    
    // ------------------------------------------------------------------------
    // draw the three axes of a LocalSpace: three lines parallel to the
    // basis vectors of the space, centered at its origin, of lengths
    // given by the coordinates of "size".
    
    
    void drawAxes  (const AbstractLocalSpace& localSpace,
                    const Vec3& size,
                    const Color& color);
    
    
    // ------------------------------------------------------------------------
    // draw the edges of a box with a given position, orientation, size
    // and color.  The box edges are aligned with the axes of the given
    // LocalSpace, and it is centered at the origin of that LocalSpace.
    // "size" is the main diagonal of the box.
    
    
    void drawBoxOutline  (const AbstractLocalSpace& localSpace,
                          const Vec3& size,
                          const Color& color);
    
    
    // ------------------------------------------------------------------------
    // draw a (filled-in, polygon-based) square checkerboard grid on the XZ
    // (horizontal) plane.
    //
    // ("size" is the length of a side of the overall checkerboard, "subsquares"
    // is the number of subsquares along each edge (for example a standard
    // checkboard has eight), "center" is the 3d position of the center of the
    // grid, color1 and color2 are used for alternating subsquares.)
    
    
    void drawXZCheckerboardGrid (const float size,
                                 const int subsquares,
                                 const Vec3& center,
                                 const Color& color1,
                                 const Color& color2);
    
    
    // ------------------------------------------------------------------------
    // draw a square grid of lines on the XZ (horizontal) plane.
    //
    // ("size" is the length of a side of the overall grid, "subsquares" is the
    // number of subsquares along each edge (for example a standard checkboard
    // has eight), "center" is the 3d position of the center of the grid, lines
    // are drawn in the specified "color".)
    
    
    void drawXZLineGrid (const float size,
                         const int subsquares,
                         const Vec3& center,
                         const Color& color);
    
    
    // ------------------------------------------------------------------------
    // Circle/disk drawing utilities
    
    
    void drawCircleOrDisk (const float radius,
                           const Vec3& axis,
                           const Vec3& center,
                           const Color& color,
                           const int segments,
                           const bool filled,
                           const bool in3d);
    
    void drawXZCircleOrDisk (const float radius,
                             const Vec3& center,
                             const Color& color,
                             const int segments,
                             const bool filled);
    
    void draw3dCircleOrDisk (const float radius,
                             const Vec3& center,
                             const Vec3& axis,
                             const Color& color,
                             const int segments,
                             const bool filled);
    
    inline void drawXZCircle (const float radius,
                              const Vec3& center,
                              const Color& color,
                              const int segments)
    {
        warnIfInUpdatePhase ("drawXZCircle");
        drawXZCircleOrDisk (radius, center, color, segments, false);
    }
    
    inline void drawXZDisk (const float radius,
                            const Vec3& center,
                            const Color& color,
                            const int segments)
    {
        warnIfInUpdatePhase ("drawXZDisk");
        drawXZCircleOrDisk (radius, center, color, segments, true);
    }
    
    inline void draw3dCircle (const float radius,
                              const Vec3& center,
                              const Vec3& axis,
                              const Color& color,
                              const int segments)
    {
        warnIfInUpdatePhase ("draw3dCircle");
        draw3dCircleOrDisk (radius, center, axis, color, segments, false);
    }
    
    inline void draw3dDisk (const float radius,
                            const Vec3& center,
                            const Vec3& axis,
                            const Color& color,
                            const int segments)
    {
        warnIfInUpdatePhase ("draw3dDisk");
        draw3dCircleOrDisk (radius, center, axis, color, segments, true);
    }
    
    
    // draw a circular arc on the XZ plane, from a start point, around a center,
    // for a given arc length, in a given number of segments and color.  The
    // sign of arcLength determines the direction in which the arc is drawn.
    
    void drawXZArc (const Vec3& start,
                    const Vec3& center,
                    const float arcLength,
                    const int segments,
                    const Color& color);
    
    
    // ------------------------------------------------------------------------
    // Sphere drawing utilities
    
    
    // draw a sphere (wireframe or opaque, with front/back/both culling)
    void drawSphere (const Vec3 center,
                     const float radius,
                     const float maxEdgeLength,
                     const bool filled,
                     const Color& color,
                     const bool drawFrontFacing = true,
                     const bool drawBackFacing = true,
                     const Vec3& viewpoint = Vec3::zero);
    
    // draw a SphereObstacle
    void drawSphereObstacle (const SphereObstacle& so,
                             const float maxEdgeLength,
                             const bool filled,
                             const Color& color,
                             const Vec3& viewpoint);
    
    
    // ------------------------------------------------------------------------
    // draw a reticle at the center of the window.  Currently it is small
    // crosshair with a gap at the center, drawn in white with black borders
    // width and height of screen are passed in
    
    
    void drawReticle (float w, float h);
    
    
    // ------------------------------------------------------------------------
    
    
    void drawBasic2dCircularVehicle (const AbstractVehicle& bv,
                                     const Color& color);
    
    void drawBasic3dSphericalVehicle (const AbstractVehicle& bv,
                                      const Color& color);
    
    void drawBasic3dSphericalVehicle (drawTriangleRoutine, const AbstractVehicle& bv,
                                      const Color& color);
    
    // ------------------------------------------------------------------------
    // 2d text drawing requires w, h since retrieving viewport w and h differs
    // for every graphics API
    
    void draw2dTextAt3dLocation (const char& text,
                                 const Vec3& location,
                                 const Color& color, float w, float h);
    
    void draw2dTextAt3dLocation (const std::ostringstream& text,
                                 const Vec3& location,
                                 const Color& color, float w, float h);
    
    void draw2dTextAt2dLocation (const char& text,
                                 const Vec3 location,
                                 const Color& color, float w, float h);
    
    void draw2dTextAt2dLocation (const std::ostringstream& text,
                                 const Vec3 location,
                                 const Color& color, float w, float h);
    
    // ------------------------------------------------------------------------
    // emit an OpenGL vertex based on a Vec3
    
    
    void glVertexVec3 (const Vec3& v);
    
    
    // ----------------------------------------------------------------------------
    // draw 3d "graphical annotation" lines, used for debugging
    
    
    void drawLine (const Vec3& startPoint,
                   const Vec3& endPoint,
                   const Color& color);
    
    
    // ----------------------------------------------------------------------------
    // draw 2d lines in screen space: x and y are the relevant coordinates
    // w and h are the dimensions of the viewport in pixels
    void draw2dLine (const Vec3& startPoint,
                     const Vec3& endPoint,
                     const Color& color,
                     float w, float h);
    
    
    // ----------------------------------------------------------------------------
    // draw a line with alpha blending
    
    void drawLineAlpha (const Vec3& startPoint,
                        const Vec3& endPoint,
                        const Color& color,
                        const float alpha);
    
    
    // ------------------------------------------------------------------------
    // deferred drawing of lines, circles and (filled) disks
    
    
    void deferredDrawLine (const Vec3& startPoint,
                           const Vec3& endPoint,
                           const Color& color);
    
    void deferredDrawCircleOrDisk (const float radius,
                                   const Vec3& axis,
                                   const Vec3& center,
                                   const Color& color,
                                   const int segments,
                                   const bool filled,
                                   const bool in3d);
    
    void drawAllDeferredLines (void);
    void drawAllDeferredCirclesOrDisks (void);
    
    
    // ------------------------------------------------------------------------
    // Draw a single OpenGL triangle given three Vec3 vertices.
    
    
    void drawTriangle (const Vec3& a,
                       const Vec3& b,
                       const Vec3& c,
                       const Color& color);
    
    
    // ------------------------------------------------------------------------
    // Draw a single OpenGL quadrangle given four Vec3 vertices, and color.
    
    
    void drawQuadrangle (const Vec3& a,
                         const Vec3& b,
                         const Vec3& c,
                         const Vec3& d,
                         const Color& color);
    
    
    // ----------------------------------------------------------------------------
    // draws a "wide line segment": a rectangle of the given width and color
    // whose mid-line connects two given endpoints
    
    
    void drawXZWideLine (const Vec3& startPoint,
                         const Vec3& endPoint,
                         const Color& color,
                         float width);
    
    
    // ----------------------------------------------------------------------------
    
    
    void drawCameraLookAt (const Vec3& cameraPosition,
                           const Vec3& pointToLookAt,
                           const Vec3& up);
    
    
    // ----------------------------------------------------------------------------
    // check for errors during redraw, report any and then exit
    
    
    void checkForDrawError (const char * locationDescription);
    
    
    
    // ----------------------------------------------------------------------------
    // return a normalized direction vector pointing from the camera towards a
    // given point on the screen: the ray that would be traced for that pixel
    
    
    Vec3 directionFromCameraToScreenPosition (int x, int y, int h);
}

// ----------------------------------------------------------------------------
// Constructor and destructor


template<class Super>
Draw::AnnotationMixin<Super>::AnnotationMixin (void)
{
    trailVertices = NULL;
    trailFlags = NULL;
    
    // xxx I wonder if it makes more sense to NOT do this here, see if the
    // xxx vehicle class calls it to set custom parameters, and if not, set
    // xxx these default parameters on first call to a "trail" function.  The
    // xxx issue is whether it is a problem to allocate default-sized arrays
    // xxx then to free them and allocate new ones
    setTrailParameters (5, 100);  // 5 seconds with 100 points along the trail
}


template<class Super>
Draw::AnnotationMixin<Super>::~AnnotationMixin (void)
{
    delete[] trailVertices;
    delete[] trailFlags;
}


// ----------------------------------------------------------------------------
// set trail parameters: the amount of time it represents and the number of
// samples along its length.  re-allocates internal buffers.


template<class Super>
void
Draw::AnnotationMixin<Super>::setTrailParameters (const float duration,
                                                       const int vertexCount)
{
    // record new parameters
    trailDuration = duration;
    trailVertexCount = vertexCount;
    
    // reset other internal trail state
    trailIndex = 0;
    trailLastSampleTime = 0;
    trailSampleInterval = trailDuration / trailVertexCount;
    trailDottedPhase = 1;
    
    // prepare trailVertices array: free old one if needed, allocate new one
    delete[] trailVertices;
    trailVertices = new Vec3[trailVertexCount];
    
    // prepare trailFlags array: free old one if needed, allocate new one
    delete[] trailFlags;
    trailFlags = new char[trailVertexCount];
    
    // initializing all flags to zero means "do not draw this segment"
    for (int i = 0; i < trailVertexCount; i++) trailFlags[i] = 0;
}


// ----------------------------------------------------------------------------
// forget trail history: used to prevent long streaks due to teleportation
//
// XXX perhaps this coudl be made automatic: triggered when the change in
// XXX position is well out of the range of the vehicles top velocity


template<class Super>
void
Draw::AnnotationMixin<Super>::clearTrailHistory (void)
{
    // brute force implementation, reset everything
    setTrailParameters (trailDuration, trailVertexCount);
}


// ----------------------------------------------------------------------------
// record a position for the current time, called once per update


template<class Super>
void
Draw::AnnotationMixin<Super>::recordTrailVertex (const float currentTime,
                                                      const Vec3& position)
{
    const float timeSinceLastTrailSample = currentTime - trailLastSampleTime;
    if (timeSinceLastTrailSample > trailSampleInterval)
    {
        trailIndex = (trailIndex + 1) % trailVertexCount;
        trailVertices [trailIndex] = position;
        trailDottedPhase = (trailDottedPhase + 1) % 2;
        const int tick = (floorXXX (currentTime) >
                          floorXXX (trailLastSampleTime));
        trailFlags [trailIndex] = trailDottedPhase | (tick ? '\2' : '\0');
        trailLastSampleTime = currentTime;
    }
    curPosition = position;
}


// ----------------------------------------------------------------------------
// draw the trail as a dotted line, fading away with age


template<class Super>
void
Draw::AnnotationMixin<Super>::drawTrail (const Color& trailColor,
                                              const Color& tickColor)
{
    if (enableAnnotation)
    {
        int index = trailIndex;
        for (int j = 0; j < trailVertexCount; j++)
        {
            // index of the next vertex (mod around ring buffer)
            const int next = (index + 1) % trailVertexCount;
            
            // "tick mark": every second, draw a segment in a different color
            const int tick = ((trailFlags [index] & 2) ||
                              (trailFlags [next] & 2));
            const Color color = tick ? tickColor : trailColor;
            
            // draw every other segment
            if (trailFlags [index] & 1)
            {
                if (j == 0)
                {
                    // draw segment from current position to first trail point
                    drawLineAlpha (curPosition,
                                   trailVertices [index],
                                   color,
                                   1);
                }
                else
                {
                    // draw trail segments with opacity decreasing with age
                    const float minO = 0.05f; // minimum opacity
                    const float fraction = (float) j / trailVertexCount;
                    const float opacity = (fraction * (1 - minO)) + minO;
                    drawLineAlpha (trailVertices [index],
                                   trailVertices [next],
                                   color,
                                   opacity);
                }
            }
            index = next;
        }
    }
}


// ----------------------------------------------------------------------------
// request (deferred) drawing of a line for graphical annotation
//
// This is called during App's simulation phase to annotate behavioral
// or steering state.  When annotation is enabled, a description of the line
// segment is queued to be drawn during App's redraw phase.


#ifdef OPENSTEERDEMO  // only when building App
template<class Super>
void
Draw::AnnotationMixin<Super>::annotationLine (const Vec3& startPoint,
                                                   const Vec3& endPoint,
                                                   const Color& color) const
{
    if (enableAnnotation)
    {
        if (drawPhaseActive)
        {
            drawLine (startPoint, endPoint, color);
        }
        else
        {
            deferredDrawLine (startPoint, endPoint, color);
        }
    }
}
#else
template<class Super> void Draw::AnnotationMixin<Super>::annotationLine
(const Vec3&, const Vec3&, const Color&) const {}
#endif // OPENSTEERDEMO


// ----------------------------------------------------------------------------
// request (deferred) drawing of a circle (or disk) for graphical annotation
//
// This is called during App's simulation phase to annotate behavioral
// or steering state.  When annotation is enabled, a description of the
// "circle or disk" is queued to be drawn during App's redraw phase.


#ifdef OPENSTEERDEMO  // only when building App
template<class Super>
void
Draw::AnnotationMixin<Super>::annotationCircleOrDisk (const float radius,
                                                           const Vec3& axis,
                                                           const Vec3& center,
                                                           const Color& color,
                                                           const int segments,
                                                           const bool filled,
                                                           const bool in3d) const
{
    if (enableAnnotation)
    {
        if (drawPhaseActive)
        {
            drawCircleOrDisk (radius, axis, center, color,
                              segments, filled, in3d);
        }
        else
        {
            deferredDrawCircleOrDisk (radius, axis, center, color,
                                      segments, filled, in3d);
        }
    }
}
#else
template<class Super>
void Draw::AnnotationMixin<Super>::annotationCircleOrDisk
(const float, const Vec3&, const Vec3&, const Color&, const int,
 const bool, const bool) const {}
#endif // OPENSTEERDEMO


// ----------------------------------------------------------------------------


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForWander (float dt)
{
    // random walk WanderSide and WanderUp between -1 and +1
    const float speed = 12.0f * dt; // maybe this (12) should be an argument?
    WanderSide = scalarRandomWalk (WanderSide, speed, -1, +1);
    WanderUp   = scalarRandomWalk (WanderUp,   speed, -1, +1);
    
    // return a pure lateral steering vector: (+/-Side) + (+/-Up)
    return (side() * WanderSide) + (up() * WanderUp);
}


// ----------------------------------------------------------------------------
// Seek behavior


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForSeek (const Vec3& target)
{
    const Vec3 desiredVelocity = target - position();
    return desiredVelocity - velocity();
}


// ----------------------------------------------------------------------------
// Flee behavior


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForFlee (const Vec3& target)
{
    const Vec3 desiredVelocity = position - target;
    return desiredVelocity - velocity();
}


// ----------------------------------------------------------------------------
// xxx proposed, experimental new seek/flee [cwr 9-16-02]


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
xxxsteerForFlee (const Vec3& target)
{
    //  const Vec3 offset = position - target;
    const Vec3 offset = position() - target;
    const Vec3 desiredVelocity = offset.truncateLength (maxSpeed ()); //xxxnew
    return desiredVelocity - velocity();
}


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
xxxsteerForSeek (const Vec3& target)
{
    //  const Vec3 offset = target - position;
    const Vec3 offset = target - position();
    const Vec3 desiredVelocity = offset.truncateLength (maxSpeed ()); //xxxnew
    return desiredVelocity - velocity();
}


// ----------------------------------------------------------------------------
// Path Following behaviors


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerToStayOnPath (const float predictionTime, Pathway& path)
{
    // predict our future position
    const Vec3 futurePosition = predictFuturePosition (predictionTime);
    
    // find the point on the path nearest the predicted future position
    Vec3 tangent;
    float outside;
    const Vec3 onPath = path.mapPointToPath (futurePosition,
                                             tangent,     // output argument
                                             outside);    // output argument
    
    if (outside < 0)
    {
        // our predicted future position was in the path,
        // return zero steering.
        return Vec3::zero;
    }
    else
    {
        // our predicted future position was outside the path, need to
        // steer towards it.  Use onPath projection of futurePosition
        // as seek target
        annotatePathFollowing (futurePosition, onPath, onPath, outside);
        return steerForSeek (onPath);
    }
}


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerToFollowPath (const int direction,
                   const float predictionTime,
                   Pathway& path)
{
    // our goal will be offset from our path distance by this amount
    const float pathDistanceOffset = direction * predictionTime * speed();
    
    // predict our future position
    const Vec3 futurePosition = predictFuturePosition (predictionTime);
    
    // measure distance along path of our current and predicted positions
    const float nowPathDistance =
    path.mapPointToPathDistance (position ());
    const float futurePathDistance =
    path.mapPointToPathDistance (futurePosition);
    
    // are we facing in the correction direction?
    const bool rightway = ((pathDistanceOffset > 0) ?
                           (nowPathDistance < futurePathDistance) :
                           (nowPathDistance > futurePathDistance));
    
    // find the point on the path nearest the predicted future position
    // XXX need to improve calling sequence, maybe change to return a
    // XXX special path-defined object which includes two Vec3s and a
    // XXX bool (onPath,tangent (ignored), withinPath)
    Vec3 tangent;
    float outside;
    const Vec3 onPath = path.mapPointToPath (futurePosition,
                                             // output arguments:
                                             tangent,
                                             outside);
    
    // no steering is required if (a) our future position is inside
    // the path tube and (b) we are facing in the correct direction
    if ((outside < 0) && rightway)
    {
        // all is well, return zero steering
        return Vec3::zero;
    }
    else
    {
        // otherwise we need to steer towards a target point obtained
        // by adding pathDistanceOffset to our current path position
        
        float const targetPathDistance = nowPathDistance + pathDistanceOffset;
        Vec3 const target = path.mapPathDistanceToPoint (targetPathDistance);
        
        annotatePathFollowing (futurePosition, onPath, target, outside);
        
        // return steering to seek target on path
        return steerForSeek (target);
    }
}


// ----------------------------------------------------------------------------
// Obstacle Avoidance behavior
//
// Returns a steering force to avoid a given obstacle.  The purely lateral
// steering force will turn our vehicle towards a silhouette edge of the
// obstacle.  Avoidance is required when (1) the obstacle intersects the
// vehicle's current path, (2) it is in front of the vehicle, and (3) is
// within minTimeToCollision seconds of travel at the vehicle's current
// velocity.  Returns a zero vector value (Vec3::zero) when no avoidance is
// required.
//
// XXX The current (4-23-03) scheme is to dump all the work on the various
// XXX Obstacle classes, making them provide a "steer vehicle to avoid me"
// XXX method.  This may well change.
//
// XXX 9-12-03: this routine is probably obsolete: its name is too close to
// XXX the new steerToAvoidObstacles and the arguments are reversed
// XXX (perhaps there should be another version of steerToAvoidObstacles
// XXX whose second arg is "const Obstacle& obstacle" just in case we want
// XXX to avoid a non-grouped obstacle)

template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerToAvoidObstacle (const float minTimeToCollision,
                      const Obstacle& obstacle)
{
    const Vec3 avoidance = obstacle.steerToAvoid (*this, minTimeToCollision);
    
    // XXX more annotation modularity problems (assumes spherical obstacle)
    if (avoidance != Vec3::zero)
        annotateAvoidObstacle (minTimeToCollision * speed());
    
    return avoidance;
}


// this version avoids all of the obstacles in an ObstacleGroup

template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerToAvoidObstacles (const float minTimeToCollision,
                       const ObstacleGroup& obstacles)
{
    const Vec3 avoidance = Obstacle::steerToAvoidObstacles (*this,
                                                            minTimeToCollision,
                                                            obstacles);
    
    // XXX more annotation modularity problems (assumes spherical obstacle)
    if (avoidance != Vec3::zero)
        annotateAvoidObstacle (minTimeToCollision * speed());
    
    return avoidance;
}


// ----------------------------------------------------------------------------
// Unaligned collision avoidance behavior: avoid colliding with other nearby
// vehicles moving in unconstrained directions.  Determine which (if any)
// other other vehicle we would collide with first, then steers to avoid the
// site of that potential collision.  Returns a steering force vector, which
// is zero length if there is no impending collision.


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerToAvoidNeighbors (const float minTimeToCollision,
                       const AVGroup& others)
{
    // first priority is to prevent immediate interpenetration
    const Vec3 separation = steerToAvoidCloseNeighbors (0, others);
    if (separation != Vec3::zero) return separation;
    
    // otherwise, go on to consider potential future collisions
    float steer = 0;
    AbstractVehicle* threat = NULL;
    
    // Time (in seconds) until the most immediate collision threat found
    // so far.  Initial value is a threshold: don't look more than this
    // many frames into the future.
    float minTime = minTimeToCollision;
    
    // xxx solely for annotation
    Vec3 xxxThreatPositionAtNearestApproach;
    Vec3 xxxOurPositionAtNearestApproach;
    
    // for each of the other vehicles, determine which (if any)
    // pose the most immediate threat of collision.
    for (AVIterator i = others.begin(); i != others.end(); i++)
    {
        AbstractVehicle& other = **i;
        if (&other != this)
        {
            // avoid when future positions are this close (or less)
            const float collisionDangerThreshold = radius() * 2;
            
            // predicted time until nearest approach of "this" and "other"
            const float time = predictNearestApproachTime (other);
            
            // If the time is in the future, sooner than any other
            // threatened collision...
            if ((time >= 0) && (time < minTime))
            {
                // if the two will be close enough to collide,
                // make a note of it
                if (computeNearestApproachPositions (other, time)
                    < collisionDangerThreshold)
                {
                    minTime = time;
                    threat = &other;
                    xxxThreatPositionAtNearestApproach
                    = hisPositionAtNearestApproach;
                    xxxOurPositionAtNearestApproach
                    = ourPositionAtNearestApproach;
                }
            }
        }
    }
    
    // if a potential collision was found, compute steering to avoid
    if (threat != NULL)
    {
        // parallel: +1, perpendicular: 0, anti-parallel: -1
        float parallelness = forward().dot(threat->forward());
        float angle = 0.707f;
        
        if (parallelness < -angle)
        {
            // anti-parallel "head on" paths:
            // steer away from future threat position
            Vec3 offset = xxxThreatPositionAtNearestApproach - position();
            float sideDot = offset.dot(side());
            steer = (sideDot > 0) ? -1.0f : 1.0f;
        }
        else
        {
            if (parallelness > angle)
            {
                // parallel paths: steer away from threat
                Vec3 offset = threat->position() - position();
                float sideDot = offset.dot(side());
                steer = (sideDot > 0) ? -1.0f : 1.0f;
            }
            else
            {
                // perpendicular paths: steer behind threat
                // (only the slower of the two does this)
                if (threat->speed() <= speed())
                {
                    float sideDot = side().dot(threat->velocity());
                    steer = (sideDot > 0) ? -1.0f : 1.0f;
                }
            }
        }
        
        annotateAvoidNeighbor (*threat,
                               steer,
                               xxxOurPositionAtNearestApproach,
                               xxxThreatPositionAtNearestApproach);
    }
    
    return side() * steer;
}



// Given two vehicles, based on their current positions and velocities,
// determine the time until nearest approach
//
// XXX should this return zero if they are already in contact?

template<class Super>
float
Draw::SteerLibraryMixin<Super>::
predictNearestApproachTime (AbstractVehicle& otherVehicle)
{
    // imagine we are at the origin with no velocity,
    // compute the relative velocity of the other vehicle
    const Vec3 myVelocity = velocity();
    const Vec3 otherVelocity = otherVehicle.velocity();
    const Vec3 relVelocity = otherVelocity - myVelocity;
    const float relSpeed = relVelocity.length();
    
    // for parallel paths, the vehicles will always be at the same distance,
    // so return 0 (aka "now") since "there is no time like the present"
    if (relSpeed == 0) return 0;
    
    // Now consider the path of the other vehicle in this relative
    // space, a line defined by the relative position and velocity.
    // The distance from the origin (our vehicle) to that line is
    // the nearest approach.
    
    // Take the unit tangent along the other vehicle's path
    const Vec3 relTangent = relVelocity / relSpeed;
    
    // find distance from its path to origin (compute offset from
    // other to us, find length of projection onto path)
    const Vec3 relPosition = position() - otherVehicle.position();
    const float projection = relTangent.dot(relPosition);
    
    return projection / relSpeed;
}


// Given the time until nearest approach (predictNearestApproachTime)
// determine position of each vehicle at that time, and the distance
// between them


template<class Super>
float
Draw::SteerLibraryMixin<Super>::
computeNearestApproachPositions (AbstractVehicle& otherVehicle,
                                 float time)
{
    const Vec3    myTravel =       forward () *       speed () * time;
    const Vec3 otherTravel = otherVehicle.forward () * otherVehicle.speed () * time;
    
    const Vec3    myFinal =       position () +    myTravel;
    const Vec3 otherFinal = otherVehicle.position () + otherTravel;
    
    // xxx for annotation
    ourPositionAtNearestApproach = myFinal;
    hisPositionAtNearestApproach = otherFinal;
    
    return Vec3::distance (myFinal, otherFinal);
}



// ----------------------------------------------------------------------------
// avoidance of "close neighbors" -- used only by steerToAvoidNeighbors
//
// XXX  Does a hard steer away from any other agent who comes withing a
// XXX  critical distance.  Ideally this should be replaced with a call
// XXX  to steerForSeparation.


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerToAvoidCloseNeighbors (const float minSeparationDistance,
                            const AVGroup& others)
{
    // for each of the other vehicles...
    for (AVIterator i = others.begin(); i != others.end(); i++)
    {
        AbstractVehicle& other = **i;
        if (&other != this)
        {
            const float sumOfRadii = radius() + other.radius();
            const float minCenterToCenter = minSeparationDistance + sumOfRadii;
            const Vec3 offset = other.position() - position();
            const float currentDistance = offset.length();
            
            if (currentDistance < minCenterToCenter)
            {
                annotateAvoidCloseNeighbor (other, minSeparationDistance);
                return (-offset).perpendicularComponent (forward());
            }
        }
    }
    
    // otherwise return zero
    return Vec3::zero;
}


// ----------------------------------------------------------------------------
// used by boid behaviors: is a given vehicle within this boid's neighborhood?


template<class Super>
bool
Draw::SteerLibraryMixin<Super>::
inBoidNeighborhood (const AbstractVehicle& otherVehicle,
                    const float minDistance,
                    const float maxDistance,
                    const float cosMaxAngle)
{
    if (&otherVehicle == this)
    {
        return false;
    }
    else
    {
        const Vec3 offset = otherVehicle.position() - position();
        const float distanceSquared = offset.lengthSquared ();
        
        // definitely in neighborhood if inside minDistance sphere
        if (distanceSquared < (minDistance * minDistance))
        {
            return true;
        }
        else
        {
            // definitely not in neighborhood if outside maxDistance sphere
            if (distanceSquared > (maxDistance * maxDistance))
            {
                return false;
            }
            else
            {
                // otherwise, test angular offset from forward axis
                const Vec3 unitOffset = offset / sqrt (distanceSquared);
                const float forwardness = forward().dot (unitOffset);
                return forwardness > cosMaxAngle;
            }
        }
    }
}


// ----------------------------------------------------------------------------
// Separation behavior: steer away from neighbors


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForSeparation (const float maxDistance,
                    const float cosMaxAngle,
                    const AVGroup& flock)
{
    // steering accumulator and count of neighbors, both initially zero
    Vec3 steering;
    int neighbors = 0;
    
    // for each of the other vehicles...
    AVIterator flockEndIter = flock.end();
    for (AVIterator otherVehicle = flock.begin(); otherVehicle != flockEndIter; ++otherVehicle )
    {
        if (inBoidNeighborhood (**otherVehicle, radius()*3, maxDistance, cosMaxAngle))
        {
            // add in steering contribution
            // (opposite of the offset direction, divided once by distance
            // to normalize, divided another time to get 1/d falloff)
            const Vec3 offset = (**otherVehicle).position() - position();
            const float distanceSquared = offset.dot(offset);
            steering += (offset / -distanceSquared);
            
            // count neighbors
            ++neighbors;
        }
    }
    
    // divide by neighbors, then normalize to pure direction
    // bk: Why dividing if you normalize afterwards?
    //     As long as normilization tests for @c 0 we can just call normalize
    //     and safe the branching if.
    /*
     if (neighbors > 0) {
     steering /= neighbors;
     steering = steering.normalize();
     }
     */
    steering = steering.normalize();
    
    return steering;
}


// ----------------------------------------------------------------------------
// Alignment behavior: steer to head in same direction as neighbors


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForAlignment (const float maxDistance,
                   const float cosMaxAngle,
                   const AVGroup& flock)
{
    // steering accumulator and count of neighbors, both initially zero
    Vec3 steering;
    int neighbors = 0;
    
    // for each of the other vehicles...
    for (AVIterator otherVehicle = flock.begin(); otherVehicle != flock.end(); otherVehicle++)
    {
        if (inBoidNeighborhood (**otherVehicle, radius()*3, maxDistance, cosMaxAngle))
        {
            // accumulate sum of neighbor's heading
            steering += (**otherVehicle).forward();
            
            // count neighbors
            neighbors++;
        }
    }
    
    // divide by neighbors, subtract off current heading to get error-
    // correcting direction, then normalize to pure direction
    if (neighbors > 0) steering = ((steering / (float)neighbors) - forward()).normalize();
    
    return steering;
}


// ----------------------------------------------------------------------------
// Cohesion behavior: to to move toward center of neighbors



template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForCohesion (const float maxDistance,
                  const float cosMaxAngle,
                  const AVGroup& flock)
{
    // steering accumulator and count of neighbors, both initially zero
    Vec3 steering;
    int neighbors = 0;
    
    // for each of the other vehicles...
    for (AVIterator otherVehicle = flock.begin(); otherVehicle != flock.end(); otherVehicle++)
    {
        if (inBoidNeighborhood (**otherVehicle, radius()*3, maxDistance, cosMaxAngle))
        {
            // accumulate sum of neighbor's positions
            steering += (**otherVehicle).position();
            
            // count neighbors
            neighbors++;
        }
    }
    
    // divide by neighbors, subtract off current position to get error-
    // correcting direction, then normalize to pure direction
    if (neighbors > 0) steering = ((steering / (float)neighbors) - position()).normalize();
    
    return steering;
}


// ----------------------------------------------------------------------------
// pursuit of another vehicle (& version with ceiling on prediction time)


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForPursuit (const AbstractVehicle& quarry)
{
    return steerForPursuit (quarry, FLT_MAX);
}


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForPursuit (const AbstractVehicle& quarry,
                 const float maxPredictionTime)
{
    // offset from this to quarry, that distance, unit vector toward quarry
    const Vec3 offset = quarry.position() - position();
    const float distance = offset.length ();
    const Vec3 unitOffset = offset / distance;
    
    // how parallel are the paths of "this" and the quarry
    // (1 means parallel, 0 is pependicular, -1 is anti-parallel)
    const float parallelness = forward().dot (quarry.forward());
    
    // how "forward" is the direction to the quarry
    // (1 means dead ahead, 0 is directly to the side, -1 is straight back)
    const float forwardness = forward().dot (unitOffset);
    
    const float directTravelTime = distance / speed ();
    const int f = intervalComparison (forwardness,  -0.707f, 0.707f);
    const int p = intervalComparison (parallelness, -0.707f, 0.707f);
    
    float timeFactor = 0; // to be filled in below
    Color color;           // to be filled in below (xxx just for debugging)
    
    // Break the pursuit into nine cases, the cross product of the
    // quarry being [ahead, aside, or behind] us and heading
    // [parallel, perpendicular, or anti-parallel] to us.
    switch (f)
    {
        case +1:
            switch (p)
        {
            case +1:          // ahead, parallel
                timeFactor = 4;
                color = gBlack;
                break;
            case 0:           // ahead, perpendicular
                timeFactor = 1.8f;
                color = gGray50;
                break;
            case -1:          // ahead, anti-parallel
                timeFactor = 0.85f;
                color = gWhite;
                break;
        }
            break;
        case 0:
            switch (p)
        {
            case +1:          // aside, parallel
                timeFactor = 1;
                color = gRed;
                break;
            case 0:           // aside, perpendicular
                timeFactor = 0.8f;
                color = gYellow;
                break;
            case -1:          // aside, anti-parallel
                timeFactor = 4;
                color = gGreen;
                break;
        }
            break;
        case -1:
            switch (p)
        {
            case +1:          // behind, parallel
                timeFactor = 0.5f;
                color= gCyan;
                break;
            case 0:           // behind, perpendicular
                timeFactor = 2;
                color= gBlue;
                break;
            case -1:          // behind, anti-parallel
                timeFactor = 2;
                color = gMagenta;
                break;
        }
            break;
    }
    
    // estimated time until intercept of quarry
    const float et = directTravelTime * timeFactor;
    
    // xxx experiment, if kept, this limit should be an argument
    const float etl = (et > maxPredictionTime) ? maxPredictionTime : et;
    
    // estimated position of quarry at intercept
    const Vec3 target = quarry.predictFuturePosition (etl);
    
    // annotation
    this->annotationLine (position(),
                          target,
                          gaudyPursuitAnnotation ? color : gGray40);
    
    return steerForSeek (target);
}

// ----------------------------------------------------------------------------
// evasion of another vehicle


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForEvasion (const AbstractVehicle& menace,
                 const float maxPredictionTime)
{
    // offset from this to menace, that distance, unit vector toward menace
    const Vec3 offset = menace.position() - position;
    const float distance = offset.length ();
    
    const float roughTime = distance / menace.speed();
    const float predictionTime = ((roughTime > maxPredictionTime) ?
                                  maxPredictionTime :
                                  roughTime);
    
    const Vec3 target = menace.predictFuturePosition (predictionTime);
    
    return steerForFlee (target);
}


// ----------------------------------------------------------------------------
// tries to maintain a given speed, returns a maxForce-clipped steering
// force along the forward/backward axis


template<class Super>
Draw::Vec3
Draw::SteerLibraryMixin<Super>::
steerForTargetSpeed (const float targetSpeed)
{
    const float mf = maxForce ();
    const float speedError = targetSpeed - speed ();
    return forward () * clip (speedError, -mf, +mf);
}

#endif
