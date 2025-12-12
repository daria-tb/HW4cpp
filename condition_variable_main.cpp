#include <random>
#include "condition_variable_custom.h"


c_condition_variable simple_cv;
std::mutex mtx;
bool ready[2] = { false, false };

void worker(int id)
{
    std::unique_lock<std::mutex> lock(mtx);
    std::cout << "Thread #" << id << " waiting...\n";
    simple_cv.wait(lock, [id] { return ready[id - 1]; });
    std::cout << "Thread #" << id << " woke up!\n";
}

int main()
{
    std::cout << "Main thread starts...\n";

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    for (int i = 0; i < 2; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready[i] = true;
            simple_cv.notify_one();
        }
    }

    t1.join();
    t2.join();

    std::cout << "Main thread ends...\n";
    return 0;
}
