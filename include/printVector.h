#include <iterator>

template <typename T>
void printVector(const std::vector<T> & vec) {
    if(vec.empty()) {
        std::cout << "vector is empty" << std::endl;
        return;
    }
    std::copy(vec.begin(),vec.end(),std::ostream_iterator<T>(std::cout," "));
    std::cout << std::endl;
}

