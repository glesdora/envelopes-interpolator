#ifndef DOUBLEBUFFER_H
#define DOUBLEBUFFER_H

#include <vector>
#include <atomic>

class DoubleBuffer {
public:
    DoubleBuffer(int size) : bufferA(size), bufferB(size), isBufferAActive(true) {}

    std::vector<float>& getActiveBuffer() {
        return isBufferAActive ? bufferA : bufferB;
    }

    std::vector<float>& getInactiveBuffer() {
        return isBufferAActive ? bufferB : bufferA;
    }

    void swapBuffers() {
        isBufferAActive = !isBufferAActive;
    }

private:
    std::vector<float> bufferA;
    std::vector<float> bufferB;
    std::atomic<bool> isBufferAActive;
};

#endif // DOUBLEBUFFER_H