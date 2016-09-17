#ifndef CODERSSTRIKEBACK_ONLINEMEDIAN_H
#define CODERSSTRIKEBACK_ONLINEMEDIAN_H

#include <queue>

/* Computes the median of a stream of data.
 * Add is O(log(n)), median is O(1).
 * Tests run on LeetCode, problem: "Find Median from Data Stream".
 **/
template<typename T>
class OnlineMedian {
public:
    std::priority_queue<T, std::vector<T>, std::greater<T> > top;
    std::priority_queue<T, std::vector<T>> bottom;
    int count = 0;
    // Adds a number into the data structure.
    void add(T num) {
        if(count == 0 || num <= bottom.top()) {
            bottom.push(num);
            if(bottom.size() > top.size() + 1) {
                top.push(bottom.top());
                bottom.pop();
            }
        } else {
            top.push(num);
            if(top.size() > bottom.size() + 1) {
                bottom.push(top.top());
                top.pop();
            }
        }
        count++;
    }

    // Returns the median of current data stream
    T median() {
        if(top.size() == 0 && bottom.size() == 0) return 1;
        if(top.size() > bottom.size()) {
            return top.top();
        } else if(bottom.size() > top.size()) {
            return bottom.top();
        } else {
            return (bottom.top()+ top.top()) / 2.0;
        }
    }
};

#endif // CODERSSTRIKEBACK_ONLINEMEDIAN_H
