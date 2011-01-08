
#include "spline.h"
#include <assert.h>
#include <limits>

using namespace G3D;
namespace Movement{

SplinePure::EvaluationMethtod SplinePure::evaluators[SplineModeCount] =
{
    &SplinePure::EvaluateLinear,
    &SplinePure::EvaluateCatmullRom,
    &SplinePure::EvaluateBezier3,
};

SplinePure::EvaluationMethtod SplinePure::hermite_evaluators[SplineModeCount] =
{
    &SplinePure::EvaluateHermiteLinear,
    &SplinePure::EvaluateHermiteCatmullRom,
    &SplinePure::EvaluateHermiteBezier3,
};

SplinePure::SegLenghtMethtod SplinePure::seglengths[SplineModeCount] =
{
    &SplinePure::SegLengthLinear,
    &SplinePure::SegLengthCatmullRom,
    &SplinePure::SegLengthBezier3,
};

SplinePure::InitMethtod SplinePure::initializers[SplineModeCount] =
{
    //&SplinePure::InitLinear,
    &SplinePure::InitCatmullRom,    // we should use catmullrom initializer even for linear mode! (client's internal structure limitation) 
    &SplinePure::InitCatmullRom,
    &SplinePure::InitBezier3,
};

void SplinePure::evaluate_percent( float t, Vector3 & c ) const
{
    index_type Index;
    float u;
    computeIndex(t, Index, u);

    (this->*evaluators[m_mode])(Index, u, c);
}

void SplinePure::evaluate_hermite(float t, Vector3& hermite) const
{
    index_type Index;
    float u;
    computeIndex(t, Index, u);

    (this->*hermite_evaluators[m_mode])(Index, u, hermite);
}

void SplinePure::evaluate_percent_and_hermite(float t, Vector3 & c, Vector3& hermite) const
{
    index_type Index;
    float u;
    computeIndex(t, Index, u);

    (this->*evaluators[m_mode])(Index, u, c);
    (this->*hermite_evaluators[m_mode])(Index, u, hermite);
}

SplinePure::index_type SplinePure::computeIndexInBounds( float length_, float t ) const
{
// Temporary disabled: causes infinite loop with t = 1.f
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
    while (i+1 < N && lengths[i+1] < length_)
        ++i;

    return i;
}

void SplinePure::computeIndex(float t, index_type& index, float& u) const
{
    assert(t >= 0.f && t <= 1.f);

    float length_ = t * length();
    index = computeIndexInBounds(length_, t);
    assert(index < index_hi);
    u = (length_ - length(index)) / segment_length(index);
}

SplinePure::index_type SplinePure::computeIndexInBounds( float t ) const
{
    float length_ = t * length();
    index_type i = index_lo;
    index_type N = index_hi;
    while (i+1 < N && lengths[i+1] < length_)
        ++i;

    return i;
}

float SplinePure::SegLength( index_type Index ) const
{
    return (this->*seglengths[m_mode])(Index);
}

///////////
#pragma region evaluation methtods

static const float s_catmullRomCoeffs[4][4]={
    -0.5f, 1.5f,-1.5f, 0.5f,
    1.f, -2.5f, 2.f, -0.5f,
    -0.5f, 0.f,  0.5f, 0.f,
    0.f,  1.f,  0.f,  0.f};

static const float s_Bezier3Coeffs[4][4]={
    -1.f,  3.f, -3.f, 1.f,
    3.f, -6.f,  3.f, 0.f,
    -3.f,  3.f,  0.f, 0.f,
    1.f,  0.f,  0.f, 0.f};

static const float g3d_catmullrom_basis[4][4]={
    0.5f, 2.f, -2.f, 0.5f,
    -1.f, -3.f, 3.f, -0.5f,
    0.5f, 0.f, 0.f, 0.f,
    -0.f, 1.f, 0.f, 0.f};

/*
inline void C_Evaluate(const Vector3 *vertice, float t, const float (&matrix)[4][4], Vector3 &position)
{
    Vector3 tvec(t*t*t, t*t, t);
    int i = 0;
    double c;
    double x = 0, y = 0, z = 0;
    while ( i < 4 )
    {
        c = matrix[0][i]*tvec.x + matrix[1][i]*tvec.y + matrix[2][i]*tvec.z + matrix[3][i];

        x += c * vertice->x;
        y += c * vertice->y;
        z += c * vertice->z;

        ++i;
        ++vertice;
    }

    position.x = x;
    position.y = y;
    position.z = z;
}*/

inline void C_Evaluate(const Vector3 *vertice, float t, const float (&matr)[4][4], Vector3 &result)
{
    float tvec[] = {t*t*t, t*t, t/*, 1.f*/};

    double matrix[4] = {
        matr[0][0]*tvec[0] + matr[1][0]*tvec[1] + matr[2][0]*tvec[2] + matr[3][0],
        matr[0][1]*tvec[0] + matr[1][1]*tvec[1] + matr[2][1]*tvec[2] + matr[3][1],
        matr[0][2]*tvec[0] + matr[1][2]*tvec[1] + matr[2][2]*tvec[2] + matr[3][2],
        matr[0][3]*tvec[0] + matr[1][3]*tvec[1] + matr[2][3]*tvec[2] + matr[3][3]
    };

    result.x = matrix[0]*vertice[0].x + matrix[1]*vertice[1].x + matrix[2]*vertice[2].x + matrix[3]*vertice[3].x;
    result.y = matrix[0]*vertice[0].y + matrix[1]*vertice[1].y + matrix[2]*vertice[2].y + matrix[3]*vertice[3].y;
    result.z = matrix[0]*vertice[0].z + matrix[1]*vertice[1].z + matrix[2]*vertice[2].z + matrix[3]*vertice[3].z;
}

inline void C_Evaluate_Hermite(const Vector3 *vertice, float t, const float (&matr)[4][4], Vector3 &result)
{
    float tvec[] = {3.f*t*t, 2.f*t/*, 1.f, 0.f*/};

    double coeff_diffs[4] = {
        matr[0][0]*tvec[0] + matr[1][0]*tvec[1] + matr[2][0],
        matr[0][1]*tvec[0] + matr[1][1]*tvec[1] + matr[2][1],
        matr[0][2]*tvec[0] + matr[1][2]*tvec[1] + matr[2][2],
        matr[0][3]*tvec[0] + matr[1][3]*tvec[1] + matr[2][3]
    };

    result.x = vertice[0].x*coeff_diffs[0] + vertice[1].x*coeff_diffs[1] +
        vertice[2].x*coeff_diffs[2] + vertice[3].x*coeff_diffs[3];

    result.y = vertice[0].y*coeff_diffs[0] + vertice[1].y*coeff_diffs[1] +
        vertice[2].y*coeff_diffs[2] + vertice[3].y*coeff_diffs[3];

    result.z = vertice[0].z*coeff_diffs[0] + vertice[1].z*coeff_diffs[1] +
        vertice[2].z*coeff_diffs[2] + vertice[3].z*coeff_diffs[3];
}

void SplinePure::EvaluateLinear(index_type Idx, float u, Vector3& result) const
{
    assert(Idx >= 0 && Idx+1 < points.size());
    result = points[Idx] + (points[Idx+1] - points[Idx]) * u;
}

void SplinePure::EvaluateCatmullRom( index_type Index, float t, Vector3& result) const
{
    assert(Index-1 >= 0 && Index+2 < points.size());
    C_Evaluate(&points[Index - 1], t, s_catmullRomCoeffs, result);
}

void SplinePure::EvaluateBezier3(index_type Index, float t, Vector3& result) const
{
    Index *= 3u;
    assert(Index >= 0 && Index+3 < points.size());
    C_Evaluate(&points[Index], t, s_Bezier3Coeffs, result);
}

void SplinePure::EvaluateHermiteLinear(index_type Index, float, Vector3& result) const
{
    assert(Index >= 0 && Index+1 < points.size());
    result = points[Index+1] - points[Index];
}

void SplinePure::EvaluateHermiteCatmullRom(index_type Index, float t, Vector3& result) const
{
    assert(Index-1 >= 0 && Index+2 < points.size());
    C_Evaluate_Hermite(&points[Index - 1], t, s_catmullRomCoeffs, result); 
}

void SplinePure::EvaluateHermiteBezier3(index_type Index, float t, Vector3& result) const
{
    assert(Index-1 >= 0 && Index+2 < points.size());
    C_Evaluate_Hermite(&points[Index - 1], t, s_Bezier3Coeffs, result); 
}

float SplinePure::SegLengthLinear(index_type i) const
{
    assert(i >= 0 && i+1 < points.size());
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

SplinePure::SplinePure() : m_mode(SplineModeLinear)
{
    index_lo = 0;
    index_hi = 0;
    points_count = 0;
}

void SplinePure::init_spline( const Vector3 * controls, const int count, SplineMode m )
{
    m_mode = m;
    points_count = count;
    cyclic = false;

    (this->*initializers[m_mode])(controls, count, cyclic, 0);
    cacheLengths();
}

void SplinePure::init_cyclic_spline( const Vector3 * controls, const int count, SplineMode m, int cyclic_point )
{
    m_mode = m;
    points_count = count;
    cyclic = true;

    (this->*initializers[m_mode])(controls, count, cyclic, cyclic_point);
    cacheLengths();
}

void SplinePure::InitLinear( const Vector3* controls, const int count, bool cyclic, int cyclic_point )
{
    assert(count >= 2);
    const int real_size = count + 1;

    points.resize(real_size);
    lengths.resize(real_size,0.f);

    memcpy(&points[0],controls, sizeof(Vector3) * count);

    // first and last two indexes are space for special 'virtual points'
    // these points are required for proper C_Evaluate and C_Evaluate_Hermite methtod work
    if (cyclic)
        points[count] = controls[cyclic_point];
    else
        points[count] = controls[count-1];

    index_lo = 0;
    index_hi = cyclic ? count : (count - 1);
}

void SplinePure::InitCatmullRom( const Vector3* controls, const int count, bool cyclic, int cyclic_point )
{
    const int real_size = count + (cyclic ? (1+2) : (1+1));

    points.resize(real_size);
    lengths.resize(real_size,0.f);

    int lo_idx = 1;
    int high_idx = lo_idx + count - 1; 

    memcpy(&points[lo_idx],controls, sizeof(Vector3) * count);

    // first and last two indexes are space for special 'virtual points'
    // these points are required for proper C_Evaluate and C_Evaluate_Hermite methtod work
    if (cyclic)
    {
        if (cyclic_point == 0)
            points[0] = controls[count-1];
        else
            points[0] = controls[0];

        points[high_idx+1] = controls[cyclic_point];
        points[high_idx+2] = controls[cyclic_point+1];
    }
    else
    {
        points[0] = controls[0];
        points[high_idx+1] = controls[count-1];
    }

    index_lo = lo_idx;
    index_hi = high_idx + (cyclic ? 1 : 0);
}

void SplinePure::InitBezier3( const Vector3* controls, const int count, bool cyclic, int cyclic_point )
{
    index_type c = count / 3u * 3u;
    index_type t = c / 3u;

    points.resize(c);
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
    while(i < index_hi )
    {
        float l = SegLength(i);

        // little trick:
        // there are some paths provided by DB where all points have same coords!
        // as a result - SplinePure interpolates position with NaN coords
        if ( l == 0.f )
            l = std::numeric_limits<float>::min();

        length += l;
        lengths[i+1] = length;
        ++i;
    }
}

void SplinePure::clear()
{
    index_lo = 0;
    index_hi = 0;
    points_count = 0;

    points.clear();
    lengths.clear();
}

void SplinePure::erase( index_type i )
{
    assert(false && "SplinePure::erase is in dev. state, it shouldn't be used");
    assert(index_lo >= i && i <= index_hi);

/*
    PointsArray copy;
    copy.reserve(points_count-1);

    std::vector<Vector3>::iterator it = points.begin()+index_lo;
    copy.insert(copy.end(), it, it + i);
    copy.insert(copy.end(), it + i, it + points_count);


    --points_count;
    (this->*initializers[m_mode])(&copy[0], copy.size(), cyclic, 0);
    cacheLengths();
*/
}

{

}

}
