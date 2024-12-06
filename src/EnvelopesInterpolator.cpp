#include "EnvelopesInterpolator.h"

EnvelopesInterpolator::EnvelopesInterpolator(int envsize) : _numberOfShapes(0), _envsize(envsize)
{
}

EnvelopesInterpolator::EnvelopesInterpolator(EnvelopeTable e) : _numberOfShapes(e.numberOfShapes), _envsize(e.envsize)
{
    if (e.peaks.size() != _numberOfShapes) return;

    _peaks = std::vector<int>(e.peaks);

    _shapes.resize(_numberOfShapes);
    for (int i = 0; i < _numberOfShapes; i++) {
        _shapes[i].resize(_envsize);
        for (int j = 0; j < _envsize; j++) {
            _shapes[i][j] = e.data[i * _envsize + j];
        }
    }
}

void EnvelopesInterpolator::interpolate(float s, float* targetbuffer)
{
    /*
        Interpolation Logic:
        1. Shapes A and B are interpolated based on a factor, `s`, where 0.0 ≤ s < _numberOfShapes.
        - The integer part of `s` determines the indices of shapes A and B between which the interpolation occurs.
            For example, if `s = 1.5`, shape A is at index 1, and shape B is at index 2.
        - The fractional part of `s` represents the interpolation factor between these shapes.
            - When the fractional part is 0, the result matches shape A.
            - When the fractional part is close to 1, the result approaches shape B.

        2. The peak position of the new shape is calculated as the weighted average of the peak positions
           of shapes A and B. 

        3. Each shape is split into two sections at its respective peak position.

        4. These sections are stretched or shrunk independently to align with the new peak position.

        5. The adjusted sections from both shapes are recombined to form complete, intermediate curves.

        6. A linear interpolation is performed between the recombined curves to produce the final shape.
    */

    if (s < 0 || s >= _numberOfShapes) return;
    
    int s_int = (int)s;
    float s_dec = s - s_int;

    // If s is an integer, return the corresponding shape
    if (s_dec == 0) {
        for (int i = 0; i < _envsize; i++) {
            targetbuffer[i] = _shapes[s_int][i];
        }
        return;
    }

    //peak position of the new shape
    int peak;

    //left/right parts of shapes A and B
    std::vector<float> l_A;
    std::vector<float> r_A;
    std::vector<float> l_B;
    std::vector<float> r_B;

    //new shapes A and B, stretched and shrunk to match the new peak position
    std::vector<float> stretched_A;
    std::vector<float> stretched_B;

    int i1 = s_int;
    int i2 = (s_int + 1) % _numberOfShapes;

    peak = (int)((_peaks[i2] - _peaks[i1]) * s_dec + _peaks[i1]);

    if (peak != 0 && peak != _envsize - 1) {
        l_A = std::vector<float>(_shapes[i1].begin(), _shapes[i1].begin() + _peaks[i1] + 1);
        r_A = std::vector<float>(_shapes[i1].begin() + _peaks[i1], _shapes[i1].end());
        l_B = std::vector<float>(_shapes[i2].begin(), _shapes[i2].begin() + _peaks[i2] + 1);
        r_B = std::vector<float>(_shapes[i2].begin() + _peaks[i2], _shapes[i2].end());

        // If one of the source peaks is at the beginning or end of the envelope, we need to add a 0 to the "external" side
        // This way, once the peak is moved, the interpolation will produce a ramp to it
        if (l_A.size() == 1) 
            l_A.insert(l_A.begin(), 0.0f);
        if (r_A.size() == 1)
            r_A.push_back(0.0f);
        if (l_B.size() == 1)
            l_B.insert(l_B.begin(), 0.0f);
        if (r_B.size() == 1)
            r_B.push_back(0.0f);

        std::vector<float> stretched_A_1;
        std::vector<float> stretched_A_2;
        std::vector<float> stretched_B_1;
        std::vector<float> stretched_B_2;
        stretchCurve(l_A, stretched_A_1, peak + 1);
        stretchCurve(l_B, stretched_A_2, peak + 1);
        stretchCurve(r_A, stretched_B_1, _envsize - peak);
        stretchCurve(r_B, stretched_B_2, _envsize - peak);
  
        stretched_A = std::vector<float>(stretched_A_1);
        stretched_A.insert(stretched_A.end()-1, stretched_B_1.begin(), stretched_B_1.end());
        stretched_B = std::vector<float>(stretched_A_2);
        stretched_B.insert(stretched_B.end()-1, stretched_B_2.begin(), stretched_B_2.end());
    } else {
        // The calculated peak can be at the beginning or end of the envelope, only if
        // the two source peaks coincide at the beginning or end of the envelope
        // in this case, there's no need to split the shapes, we just interpolate between them.

        stretched_A = std::vector<float>(_shapes[i1]);
        stretched_B = std::vector<float>(_shapes[i2]);
    }
    
    // Interpolate between stretched_A and stretched_B
    for (int i = 0; i < _envsize; i++) {
        targetbuffer[i] = (1 - s_dec) * stretched_A[i] + s_dec * stretched_B[i];
    }
}

void EnvelopesInterpolator::interpolate(float s, std::vector<float>& targetbuffer)
{
    if (targetbuffer.size() < _envsize) return;
    interpolate(s, targetbuffer.data());
}

void EnvelopesInterpolator::stretchCurve(const std::vector<float>& inputCurve, std::vector<float>& stretchedCurve, int newLength)
{
    int originalLength = static_cast<int>(inputCurve.size()) - 1;
    stretchedCurve.resize(newLength);

    for (int x = 0; x < newLength; ++x) {
        // Find the corresponding "virtual" x-coordinate in the original curve
        float originalX = static_cast<float>(x) * originalLength / (newLength - 1);

        // Find the two points that the originalX lies between
        int x0 = static_cast<int>(originalX);
        int x1 = std::min(x0 + 1, originalLength); // Ensure we don't go out of bounds

        // Get the y-values of these points
        float y0 = inputCurve[x0];
        float y1 = inputCurve[x1];

        // Linear interpolation to find the y-value at originalX
        float t = originalX - x0; // Fractional part of originalX
        stretchedCurve[x] = y0 + t * (y1 - y0);
    }
}

//set new data and peaks, with data being a one dimensional array of size numberOfShapes*envsize
void EnvelopesInterpolator::setDataAndPeaks(const float* data, const std::vector<int>& peaks)
{
    if (data == nullptr) return;
    
    _numberOfShapes = peaks.size();

    _shapes.resize(_numberOfShapes);
    for (int n = 0; n < _numberOfShapes; n++) {
        _shapes[n].resize(_envsize);
        for (int i = 0; i < _envsize; i++) {
            _shapes[n][i] = data[n * _envsize + i];
        }
    }

    _peaks = std::vector<int>(peaks);
}

//set new data, peaks and envsize
void EnvelopesInterpolator::setEnvelopeTable(EnvelopeTable e)
{
    if (e.data == nullptr) return;
    if (e.numberOfShapes != e.peaks.size()) return;

    _envsize = e.envsize;
    _numberOfShapes = e.numberOfShapes;
    _peaks = std::vector<int>(e.peaks);

    _shapes.resize(_numberOfShapes);
    for (int n = 0; n < _numberOfShapes; n++) {
        _shapes[n].resize(_envsize);
        for (int i = 0; i < _envsize; i++) {
            _shapes[n][i] = e.data[n * _envsize + i];
        }
    }
}

//add a new shape, drawn via linear interpolation between given points, at the end of the table
void EnvelopesInterpolator::addLinearShape(const std::vector<std::pair<int, float>>& points, int peakPosition)
{
    if (points[0].first != 0 || points[points.size()-1].first != _envsize - 1) return;

    std::vector<float> newshape(_envsize);

    for (size_t i = 0; i < points.size() - 1; ++i) {
        int x0 = points[i].first;
        float y0 = points[i].second;
        int x1 = points[i + 1].first;
        float y1 = points[i + 1].second;

        for (int x = x0; x <= x1; ++x) {
            float t = static_cast<float>(x - x0) / (x1 - x0);
            newshape[x] = y0 + t * (y1 - y0);
        }
    }

    _shapes.push_back(newshape);
    _numberOfShapes++;
    _peaks.push_back(peakPosition);
}