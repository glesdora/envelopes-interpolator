#include <iostream>
#include <vector>
#include "DoubleBuffer.h"
#include "EnvelopesInterpolator.h"

void printenvelope(const std::vector<float>& envelope, int size)
{
    const int height = 12; // Vertical scale for visualization

    std::cout << "\nVisual Representation:\n";
    for (int i = height; i >= 0; --i) {
        std::cout << i << " | ";
        for (int x = 0; x < size; ++x) {
            if (static_cast<int>(envelope[x] * height) == i)
                std::cout << "*";
            else
                std::cout << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "    " << std::string(size, '-') << "\n     ";
}

int main()
{
    const int size = 100;
    EnvelopesInterpolator et(size);
    DoubleBuffer db(size);

    // Define shapes
    std::vector<std::pair<int, float>> s1 = { {0, 0.0f}, {3, 1.0f}, {size-6, 0.0f}, {size-3, 0.5f}, {size-1, 0.0f} };
    std::vector<std::pair<int, float>> s2 = { {0, 0.0f}, {1, 1.0f}, {size-1, 0.0f} };
    std::vector<std::pair<int, float>> s3 = { {0, 0.0f}, {size-2, 1.0f}, {size-1, 0.0f} };
    std::vector<std::pair<int, float>> s4 = { {0, 0.0f}, {15, 1.0f}, {40, 0.6f}, {80, 0.6f}, {size-1, 0.0f} };

    et.addLinearShape(s1, 3);
    et.addLinearShape(s2, 1);
    et.addLinearShape(s3, size-2);
    et.addLinearShape(s4, 15);

    // Test cases
    std::vector<float> testInputs = {0.0f, 1.5f, 2.2f, 3.3f};

    std::cout << "Testing EnvelopesInterpolator:\n";
    for (float inter : testInputs) {
        std::cout << "\nInterpolation Factor: " << inter << "\n";
        
        auto& writingBuffer = db.getInactiveBuffer();
        et.interpolate(inter, writingBuffer);
        
        printenvelope(writingBuffer, size);

        db.swapBuffers();
    }

    return 0;
}