#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <chrono>

std::string check_assignment(std::string student_name, int score_to_get) 
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return "Студент " + student_name + " отримав " + std::to_string(score_to_get) + " балів";
}

int main() 
{
    auto f1 = std::async(std::launch::async, check_assignment, "Даша", 90);
    auto f2 = std::async(std::launch::async, check_assignment, "Маша", 100);

    std::cout << "головний потік: оцінюю наступне завдання..." << std::endl;

    std::string res1 = f1.get();
    std::string res2 = f2.get();

    std::cout << res1 << std::endl;
    std::cout << res2 << std::endl;

    return 0;
}