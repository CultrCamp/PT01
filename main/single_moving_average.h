//
// Created by HanWool Lee on 24. 12. 31.
//

#ifndef PIPE_TEMP_SENSOR_SINGLE_MOVING_AVERAGE_H
#define PIPE_TEMP_SENSOR_SINGLE_MOVING_AVERAGE_H
#include <queue>
#include <numeric>

template <typename T>
class MovingAverage {
private:
    std::queue<T> window;
    int windowSize;
    T sum;

public:
    MovingAverage(int size) : windowSize(size), sum(0) {}

    T next(T value) {
        sum += value;
        window.push(value);

        if (window.size() > windowSize) {
            sum -= window.front();
            window.pop();
        }

        return sum / window.size();
    }
};
#endif //PIPE_TEMP_SENSOR_SINGLE_MOVING_AVERAGE_H
