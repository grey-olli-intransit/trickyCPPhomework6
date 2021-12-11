#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <mutex>
//#include <cassert>
#include "printVector.h"

auto programStartedHighResTime = std::chrono::high_resolution_clock::now();
pthread_mutex_t mutexForSharedVector;

#define WORKER_STEPS 35

/*
 * Реализовать функцию, возвращающую i-ое простое число
 * (например, миллионное простое число равно 15485863).
 * Вычисления реализовать во вторичном потоке.
 * В консоли отображать прогресс вычисления.
*/

bool isSimpleNumber(int n) {
    bool  n_is_simple = true;
    if (n==0) return false;
    if ((n==1) || (n == 2)) return true;
    for(int i=2; i<n;++i) {
        if (n % i == 0) {
            n_is_simple= false;
            break;
        }
    }
    return n_is_simple;
}

int getNthSimpleNumber(int n) {
    int counter=0; // 1 is 1st simple
    if(n==0) return 0;
    if(n==1) return 1;
    int simpleNumberToReturn=0;
    for(int i=1;i<=4*n;++i) {
        if(isSimpleNumber(i)) {
            std::cout << "counting simple number (" << i <<").";
            ++counter;
            if (counter==n) {
                simpleNumberToReturn=i;
                break;
            }
        }
    }
    return simpleNumberToReturn;
}

void checkCalculation() {
//    std::vector<std::pair<int,int>> simpleBelow100withCounter{{2,2},{3,5},{5,7},{7,9},
//                                                              {11,12},{13,14}};
    std::vector<int> simpleBelow100{2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73,
                                    79, 83, 89, 97};
    std::vector<int> nonSimpleBelow17{0,4,6,8,9,10,12,14,15,96,95,90};
    std::cout << " Precalculated check. " << std::endl;
    std::cout << "simple:" << std::endl;
    for (auto i:simpleBelow100) {
        std::cout << i << " " << std::boolalpha << isSimpleNumber(i) << std::endl;
    }
    std::cout << "non-simple:" << std::endl;
    for (auto i:nonSimpleBelow17) {
        std::cout << i << " " << std::boolalpha << isSimpleNumber(i) << std::endl;
    }

}


void print_seconds_gone() {
    auto now = std::chrono::high_resolution_clock::now();
    auto difference = std::chrono::duration_cast<std::chrono::seconds>(now - programStartedHighResTime).count();
    std::cout << "\t\t\t\t\t\t\t\tSeconds since start: " << difference << std::endl;
}

/* Промоделировать следующую ситуацию:
 * Есть два человека (2 потока): хозяин и вор.
 * Хозяин приносит домой вещи (функция добавляющая случайное число в вектор с периодичностью 1 раз в секунду).
 * При этом у каждой вещи есть своя ценность.
 * Вор забирает вещи (функция, которая находит наибольшее число и удаляет из вектора с периодичностью 1 раз в
 * 0.5 секунд), каждый раз забирает вещь с наибольшей ценностью.
 **/
void ownerAppendRandomValuePerSecond(std::vector<int> & store) {
    int exit_flag=0;
    while (exit_flag < WORKER_STEPS) {
        ++exit_flag;
        int randomValue=std::abs((std::rand() * 10 + 1) % 10);
        std::chrono::seconds s{1};
        std::this_thread::sleep_for(s);
        pthread_mutex_lock(&mutexForSharedVector);
        // this asserts.
        //        bool noExceptionCanArrive=noexcept(store.emplace_back(randomValue));
        //        assert(noExceptionCanArrive);
        try {
            store.emplace_back(randomValue);
        } catch (...) {
            // this has not happened while testing (see above 'this asserts')
            std::cout << "Warning! Exception while emplace_back()." << std::endl;
        }
        std::cout << "owner: added value: " << randomValue << ", steps left: "
                  << WORKER_STEPS - exit_flag  << ", steps passed: " << exit_flag << std::endl;
        print_seconds_gone();
        pthread_mutex_unlock(&mutexForSharedVector);
    }
    pthread_mutex_lock(&mutexForSharedVector);
    std::cout << "owner ended." << std::endl;
    pthread_mutex_unlock(&mutexForSharedVector);

}

void thiefRemoveHighestValuePerHalfSecond(std::vector<int> & store) {
    std::chrono::milliseconds milliSec{500};
    int exit_flag=0;
    while (exit_flag < WORKER_STEPS) {
        ++exit_flag;

        std::this_thread::sleep_for(milliSec);
        pthread_mutex_lock(&mutexForSharedVector);
        //bool noExceptionCanArrive=noexcept(
        try {

            std::sort(store.begin(), store.end(),
                      [](const int &left, const int &right) {
                          return left < right;
                      }
            );
        } catch (...) {
            std::cout << "Warning! Exception arrived. (sort)" << std::endl;
        }

        //); // если тут использовать обёртывание в noexcept вместо try/catch - не хочет съесть точно такую лямбду.
        //assert(noExceptionCanArrive);
        try {
            if (!store.empty()) {
                std::cout << "thief: stealing value: " << *(store.end()-1) << std::endl;
                store.pop_back();
            }
            std::cout << "thief: steps left: " << WORKER_STEPS - exit_flag  << ", steps passed: " <<
                          exit_flag << std::endl;
        } catch (...) {
            std::cout << "Warning! Exception arrived again. (pop_back)" << std::endl;
        }

        try {
            printVector(store);
        } catch (...) {
            std::cout << "Warning! Exception arrived once again. (print)" << std::endl;
        }

        pthread_mutex_unlock(&mutexForSharedVector);
    } // while exit_flag
    pthread_mutex_lock(&mutexForSharedVector);
    std::cout << "thief ended." << std::endl;
    pthread_mutex_unlock(&mutexForSharedVector);
}



/* Создайте потокобезопасную оболочку для объекта cout. Назовите ее pcout.
 * Необходимо, чтобы несколько потоков могли обращаться к pcout и информация выводилась в консоль.
 * Продемонстрируйте работу pcout.
 **/
class Pcout  {
    static std::mutex mutexForCOUT;
public:

    template<class T>
    Pcout& operator<<( const T & toPrint) {
        // error: use of class template 'std::lock_guard' requires template arguments
        std::lock_guard <std::mutex> lockGuard(Pcout::mutexForCOUT); // добавлено <std::mutex>
        std::cout << toPrint;
        return *this;
    }

    // эти, согласно stackoverflow - для std::endl и т.п.
    friend Pcout& operator<<(Pcout &s, std::ostream& (*f)(std::ostream &));
    friend Pcout& operator<<(Pcout &s, std::ostream& (*f)(std::ios &));
    friend Pcout& operator<<(Pcout &s, std::ostream& (*f)(std::ios_base &));
};

Pcout& operator<<(Pcout &s, std::ostream& (*f)(std::ostream &)) {
    f(std::cout);
    return s;
}

Pcout& operator<<(Pcout &s, std::ostream& (*f)(std::ios &)) {
    f(std::cout);
    return s;
}

Pcout& operator<< (Pcout &s, std::ostream& (*f)(std::ios_base &)) {
    f(std::cout);
    return s;
}


std::mutex Pcout::mutexForCOUT{};

void print_str1(Pcout & out) {
    std::chrono::seconds s{abs(std::rand()*10) % 10};
    std::this_thread::sleep_for(s);
    out << "print_str1\n";
}

void print_str2(Pcout & out1) {
    std::chrono::seconds s{abs(std::rand()*10) % 10};
    std::this_thread::sleep_for(s);
    out1 << "print_str2" << std::endl;
}

int main() {

    Pcout pcout;
    std::thread OneThread([&pcout](){print_str1(pcout);});
    std::thread OtherThread([&pcout](){print_str2(pcout);});
    OneThread.join();
    OtherThread.join();

    std::vector<int> sharedHome{1,2,4,5,3,6,7,8,9,9,10,11,12,13,14};
    std::cout << "Initially items in shared home:" << std::endl;
    printVector(sharedHome);
    std::cout << "Each worker will end after " << WORKER_STEPS << " steps." << std::endl;
    std::thread ownerThread(ownerAppendRandomValuePerSecond, std::ref(sharedHome));
    std::thread thiefThread(thiefRemoveHighestValuePerHalfSecond, std::ref(sharedHome));
    ownerThread.detach();
    thiefThread.detach();
    std::chrono::seconds seconds_{20};
    std::this_thread::sleep_for(seconds_);
    std::cout << "Main thread awaken." << std::endl;
    if(ownerThread.joinable())
        ownerThread.join();
    else
        std::cout << "ownerThread is not joinable after detach()." << std::endl;
    if(thiefThread.joinable())
        thiefThread.join();
    else
        std::cout << "thiefThread is not joinable after detach()." << std::endl;
    std::cout << "Finally, items in shared home:" << std::endl;
    pthread_mutex_lock(&mutexForSharedVector);
    printVector(sharedHome);
    pthread_mutex_unlock(&mutexForSharedVector);

    std::cout << "Оценка к-ва потоков которые могут выполняться действительно одновременно: "
              << std::thread::hardware_concurrency() << std::endl;

    checkCalculation();

    std::cout << " number | value" << std::endl;
    for (int i = 1; i < 20; ++i) {
        int result;
        std::thread newThread([&result](int index){result = getNthSimpleNumber(index);},i);
        newThread.join();
        std::cout << std::endl << "      " << i << " | " << result << std::endl;
    }

    return 0;
}