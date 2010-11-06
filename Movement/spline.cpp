
#include "spline.h"
#include "g3d\matrix4.h"
#include "g3d\vector4.h"
#include "outLog.h"
#include "mov_constants.h"

using namespace G3D;
namespace Movement{

SplinePure::InterpolatorPtr SplinePure::interpolators[SplineModeCount] =
{
    &SplinePure::InterpolateLinear,
    &SplinePure::InterpolateCatmullRom,
    &SplinePure::InterpolateBezier3,
};

SplinePure::SegLenghtPtr SplinePure::seglengths[SplineModeCount] =
{
    &SplinePure::SegLengthLinear,
    &SplinePure::SegLengthCatmullRom,
    &SplinePure::SegLengthBezier3,
};

SplinePure::InitPathPtr SplinePure::initializers[SplineModeCount] =
{
    &SplinePure::InitLinear,
    &SplinePure::InitCatmullRom,
    &SplinePure::InitBezier3,
};

void SplinePure::evaluate(time_type time, Vector3 & c ) const
{
    assert(time >= 0);

    index_type Index = index_lo;
    float u = 0.f;
    computeIndex(index_lo, Index, time, u);

    (this->*interpolators[m_mode])(Index, u, c);

    movLog.write("%f   %f", c.x, c.y);
}

void SplinePure::evaluate( time_type time, Vector3 & c, time_type & out) const
{
    assert(time >= 0);

    index_type Index = index_lo;
    float u = 0.f;
    computeIndex(index_lo, Index, time, u);
    out = time;

    (this->*interpolators[m_mode])(Index, u, c);

    movLog.write("%f   %f", c.x, c.y);
}

void SplinePure::evaluate_percent( float t, Vector3 & c ) const
{
    assert(perc >= 0 && perc <= 1.f);

    float length = t * full_length;
    index_type Index = computeIndexInBounds(length, t);
    assert(Index < index_hi);
    float u = (length - lengths[Index]) / (lengths[Index+1] - lengths[Index]);

    (this->*interpolators[m_mode])(Index, u, c);

    movLog.write("%f   %f", c.x, c.y);
}

void SplinePure::computeIndex( index_type lastIndex, index_type& Index, time_type &X, float &percent) const
{
    time_type high = hight_bound();
    time_type low = low_bound();

    if (low > X || X >= high)  // X is out of bounds?
    {
        if (!cyclic)
        {
            percent = 0.f;
            if( X < low)
            {
                Index = index_lo;
                X = low;
            }
            else
            {
                Index = index_hi;
                X = high;
            }
            return;
        }
        else
        {
            X = X % duration();    // X now in range [ 0; duration )
            lastIndex = index_lo;
        }
    }

    Index = computeIndexInBounds(X);
    assert(Index < index_hi);
    percent = float(X - times[Index]) / float(times[Index+1] - times[Index]);
}

template<class Y, class X> inline Y interporate(const Y * a, X ia,  X I)
{
    return ( a[0] + (a[1] - a[0]) / 1 * (I - ia) );
}

SplinePure::index_type SplinePure::computeIndexInBounds( index_type lastIdx, const time_type time_passed_delta ) const
{
    index_type N = times.size();
    while (lastIdx+1 < N && times[lastIdx+1] < time_passed_delta)
    {
        ++lastIdx;
    }
    return lastIdx;
}

SplinePure::index_type SplinePure::computeIndexInBounds( time_type t ) const
{
/* Temporary disabled: causes deadloop with t = times[index_hi]
    index_type hi = index_hi;
    index_type lo = index_lo;

    index_type i = lo + (hi - lo) * (float)(t - low_bound()) / (float)duration();

    while ((times[i] > t) || (times[i + 1] <= t))
    {
        if (times[i] > t)
            hi = i - 1; // too big
        else if (times[i + 1] <= t)
            lo = i + 1; // too small

        i = (hi + lo) / 2;
    }*/
    index_type i = index_lo;
    index_type N = index_hi;
    while (i+1 < N && times[i+1] < t)
    {
        ++i;
    }

    return i;
}

SplinePure::index_type SplinePure::computeIndexInBounds( float length, float t ) const
{
// Temporary disabled: causes deadloop with t = 1.f
/*
    index_type hi = index_hi;
    index_type lo = index_lo;

    index_type i = lo + (float)(hi - lo) * t;

    while ((lengths[i] > length) || (lengths[i + 1] <= length))
    {
        if (lengths[i] > length)
            hi = i - 1; // too big
        else if (lengths[i + 1] <= length)
            lo = i + 1; // too small

        i = (hi + lo) / 2;
    }*/

    index_type i = index_lo;
    index_type N = index_hi;
    while (i+1 < N && lengths[i+1] < length)
    {
        ++i;
    }

    return i;
}

float SplinePure::SegLength( index_type Index ) const
{
    return (this->*seglengths[m_mode])(Index);
}

///////////
#pragma region evaluation methtods

static const G3D::Matrix4 s_catmullRomCoeffs(
    -0.5f, 1.5f,-1.5f, 0.5f,
    1.f, -2.5f, 2.f, -0.5f,
    -0.5f, 0.f,  0.5f, 0.f,
    0.f,  1.f,  0.f,  0.f);

static const G3D::Matrix4 s_Bezier3Coeffs(
    -1.f,  3.f, -3.f, 1.f,
    3.f, -6.f,  3.f, 0.f,
    -3.f,  3.f,  0.f, 0.f,
    1.f,  0.f,  0.f, 0.f);

static const G3D::Matrix4 g3d_catmullrom_basis(
    0.5f, 2.f, -2.f, 0.5f,
    -1.f, -3.f, 3.f, -0.5f,
    0.5f, 0.f, 0.f, 0.f,
    -0.f, 1.f, 0.f, 0.f);

inline void C_Evaluate(const Vector3 *vertice, float t, const Matrix4& coeffs, Vector3 &position)
{
    Vector3 tvec(t*t*t, t*t, t);
    int i = 0;
    double c;
    double x = 0, y = 0, z = 0;
    while ( i < 4 )
    {
        c = coeffs[0][i]*tvec.x + coeffs[1][i]*tvec.y + coeffs[2][i]*tvec.z + coeffs[3][i];

        x += c * vertice->x;
        y += c * vertice->y;
        z += c * vertice->z;

        ++i;
        ++vertice;
    }

    position.x = x;
    position.y = y;
    position.z = z;
}

void SplinePure::InterpolateLinear(index_type Idx, float u, Vector3& result) const
{
    assert(Index >= 0 && Index+1 < points.size());
    result = points[Idx] + (points[Idx+1] - points[Idx]) * u;
}

void SplinePure::InterpolateCatmullRom( index_type Index, float t, Vector3& result) const
{
    assert(Index-1 >= 0 && Index+2 < points.size());
    C_Evaluate(&points[Index - 1], t, s_catmullRomCoeffs, result);
}

void SplinePure::InterpolateBezier3(index_type Index, float t, Vector3& result) const
{
    Index *= 3u;
    assert(Index >= 0 && Index+3 < points.size());
    C_Evaluate(&points[Index], t, s_Bezier3Coeffs, result);
}

float SplinePure::SegLengthLinear(index_type i) const
{
    assert(Index >= 0 && Index+1 < points.size());
    return (points[i] - points[i+1]).length();
}

float SplinePure::SegLengthCatmullRom( index_type Index ) const
{
    assert(Index-1 >= 0 && Index+2 < points.size());

    Vector3 curPos, nextPos;
    const Vector3 * p = &points[Index - 1];
    curPos = nextPos = p[1];

    index_type i = 1;
    double length = 0;
    while (i <= STEPS_PER_SEGMENT)
    {
        C_Evaluate(p, float(i) / float(STEPS_PER_SEGMENT), s_catmullRomCoeffs, nextPos);
        length += (nextPos - curPos).length();
        curPos = nextPos;
        ++i;
    }
    return length;
}

float SplinePure::SegLengthBezier3(index_type Index) const
{
    Index *= 3u;
    assert(Index >= 0 && Index+3 < points.size());

    Vector3 curPos, nextPos;
    const Vector3 * p = &points[Index];

    C_Evaluate(p, 0.f, s_Bezier3Coeffs, nextPos);
    curPos = nextPos;

    index_type i = 1;
    double length = 0;
    while (i <= STEPS_PER_SEGMENT)
    {
        C_Evaluate(p, float(i) / float(STEPS_PER_SEGMENT), s_Bezier3Coeffs, nextPos);
        length += (nextPos - curPos).length();
        curPos = nextPos;
        ++i;
    }
    return length;
}
#pragma endregion

SplinePure::SplinePure() : cyclic(false), m_mode(SplineModeLinear)
{
    index_lo = 0;
    index_hi = 0;
    full_length = 0.f;
}

void SplinePure::init_path( const Vector3 * controls, const int count, SplineMode m, bool cyclic_ )
{
    cyclic = cyclic_;
    m_mode = m;

    (this->*initializers[m_mode])(controls, count);
    cacheLengths();
}

void SplinePure::InitLinear( const Vector3* controls, const int count )
{
    assert(count >= 2);
    const int real_size = count + 1;

    points.resize(real_size);
    times.resize(real_size,0);
    lengths.resize(real_size,0.f);

    memcpy(&points[0],controls, sizeof(Vector3) * count);

    // index 0 and last two\one indexes are space for special 'virtual points'
    // these points are required for proper C_Evaluate methtod work
    if (cyclic)
        points[count] = points[0];
    else
        points[count] = points[count-1];

    index_lo = 0;
    index_hi = cyclic ? count : (count - 1);
}

void SplinePure::InitCatmullRom( const Vector3* controls, const int count )
{
    const int real_size = count + (cyclic ? (1+2) : (1+1));

    points.resize(real_size);
    times.resize(real_size,0);
    lengths.resize(real_size,0.f);

    int lo_idx = 1;
    int high_idx = lo_idx + count - 1; 

    memcpy(&points[lo_idx],controls, sizeof(Vector3) * count);

    // index 0 and last two\one indexes are space for special 'virtual points'
    // these points are required for proper C_Evaluate methtod work
    if (cyclic)
    {
        points[0] = points[high_idx];
        points[high_idx+1] = points[lo_idx];
        points[high_idx+2] = points[lo_idx+1];
    }
    else
    {
        points[0] = points[lo_idx];
        points[high_idx+1] = points[high_idx];
    }

    index_lo = lo_idx;
    index_hi = high_idx + (cyclic ? 1 : 0);
}

void SplinePure::InitBezier3( const Vector3* controls, const int count )
{
    index_type c = count / 3u * 3u;
    index_type t = c / 3u;

    points.resize(c);
    times.resize(t,0);
    lengths.resize(t,0.f);
    memcpy(&points[0],controls, sizeof(Vector3) * c);

    index_lo = 0;
    index_hi = t-1;
    //assert(points.size() % 3 == 0);
}

void SplinePure::cacheLengths()
{
    index_type i = index_lo;
    double length = 0;
    while(i < index_hi ){
        length += SegLength(i);
        lengths[i+1] = length;
        ++i;
    }
    full_length = length;

    i = index_lo + 1;
    while(i <= index_hi){
        times[i] = lengths[i] / Movement::absolute_velocy * 1000.f;
        ++i;
    }
}

}
