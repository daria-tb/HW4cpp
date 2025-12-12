/* Умова: Розробити симулятор системи обробки фінансових замовлень. 
Система має три основні етапи:
1. Генерація (Producer)
2. Обробка (Consumer)
3. Фінальний Звіт (Reporting)#include <iostream> */
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include <vector>
#include <iomanip>

using namespace std;

struct Order 
{
    int id;
    int quantity;
    double price;
};

struct ProcessedResult 
{
    int order_id;
    int quantity;
    double price;
    double revenue;
    double expensive_score; // результат "дорогого" обчислення
};

queue<Order> order_queue;
mutex queue_mtx;
condition_variable cv_order;

//для “дорогих” результатів (щоб звіт був не тільки сумою)
vector<ProcessedResult> processed_results;
mutex results_mtx;

atomic<int> produced_count{0};
atomic<int> processed_count{0};

atomic<bool> production_done{false};

double final_total_revenue = 0.0;
mutex revenue_mtx;

constexpr int TOTAL_ORDERS_TO_PROCESS = 15;

//"дороге" обчислення (імітація CPU-навантаження)
static double expensive_calculation(int order_id, int quantity, double price) 
{
    //обчислювальний цикл щоб було відчутне навантаження
    double x = 0.000001 * (order_id + 1) + 0.01 * quantity + price * 0.001;
    double acc = 0.0;

    //не надто повільно, але відчутно
    for (int i = 0; i < 200000; ++i) 
    {
        x = x * 1.0000003 + 0.0000001;
        acc += (x / (1.0 + (i % 7))) * 0.000001;
        if (x > 10.0) x *= 0.1;
    }
    return acc;
}

void producer(int client_id, int orders_to_generate) 
{
    //окремий RNG на потік (rand() небажаний у багатопоточності)
    std::mt19937 rng(static_cast<uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
        + client_id * 2654435761u
    ));
    std::uniform_int_distribution<int> qty_dist(1, 10);
    std::uniform_real_distribution<double> price_dist(5.0, 105.0);
    std::uniform_int_distribution<int> sleep_dist(50, 70);

    cout << "Client [" << client_id << "] creating " << orders_to_generate << " orders.\n";

    for (int i = 0; i < orders_to_generate; ++i) 
    {
        Order order{ client_id * 100 + i, qty_dist(rng), price_dist(rng) };

        {
            lock_guard<mutex> lock(queue_mtx);
            order_queue.push(order);
        }

        produced_count.fetch_add(1, std::memory_order_relaxed);

        cout << "Client " << client_id << " added order #" << order.id
             << " (qty=" << order.quantity
             << ", price=" << fixed << setprecision(2) << order.price << ").\n";

        cv_order.notify_one();
        this_thread::sleep_for(chrono::milliseconds(sleep_dist(rng)));
    }
}

void consumer(int worker_id) 
{
    while (true) 
    {
        Order order;

        {
            unique_lock<mutex> lock(queue_mtx);

            //чекаємо, поки: є замовлення в черзі, або виробництво завершене і черга порожня (тоді вихід)
            cv_order.wait(lock, [] 
                {
                return !order_queue.empty() || production_done.load(std::memory_order_acquire);
            });

            if (order_queue.empty() && production_done.load(std::memory_order_acquire)) 
            {
                return;
            }

            order = order_queue.front();
            order_queue.pop();
        }

        double revenue = static_cast<double>(order.quantity) * order.price;
        double score = expensive_calculation(order.id, order.quantity, order.price);

        {
            lock_guard<mutex> lock(revenue_mtx);
            final_total_revenue += revenue;
        }

        {
            lock_guard<mutex> lock(results_mtx);
            processed_results.push_back(ProcessedResult{
                order.id, order.quantity, order.price, revenue, score
            });
        }

        int done = processed_count.fetch_add(1, std::memory_order_relaxed) + 1;

        cout << "Worker [" << worker_id << "] processed order #"
             << order.id << " -> revenue=" << fixed << setprecision(2) << revenue
             << " (processed " << done << "/" << TOTAL_ORDERS_TO_PROCESS << ").\n";
    }
}

int main() 
{
    //3 клієнти, сума = 15 замовлень
    const int clients = 5;
    const int workers = 3;

    //розклад генерації (сума має дорівнювати TOTAL_ORDERS_TO_PROCESS)
    vector<int> orders_per_client = {3, 3, 3, 3, 3 };

    int sum = 0;
    for (int x : orders_per_client) sum += x;
    if (sum != TOTAL_ORDERS_TO_PROCESS) 
    {
        cerr << "Configuration error: sum(orders_per_client) != TOTAL_ORDERS_TO_PROCESS\n";
        return 1;
    }

    cout << "Financial Orders Simulator\n";
    cout << "Clients: " << clients << ", Workers: " << workers
         << ", Total orders: " << TOTAL_ORDERS_TO_PROCESS << "\n\n";

    vector<thread> producer_threads;
    vector<thread> worker_threads;

    //запуск consumer-ів
    for (int i = 0; i < workers; ++i) 
    {
        worker_threads.emplace_back(consumer, i + 1);
    }

    //запуск producer-ів
    for (int i = 0; i < clients; ++i) 
    {
        producer_threads.emplace_back(producer, i + 1, orders_per_client[i]);
    }

    //чекаємо завершення генерації
    for (auto& t : producer_threads) 
    {
        t.join();
    }

    //сигнал що виробництво завершено
    production_done.store(true, std::memory_order_release);
    cv_order.notify_all();

    //чекаємо завершення обробки
    for (auto& t : worker_threads) 
    {
        t.join();
    }

    //Reporting
    cout << "\nFINAL REPORT\n";
    cout << "Produced orders:  " << produced_count.load() << "\n";
    cout << "Processed orders: " << processed_count.load() << "\n";

    {
        lock_guard<mutex> lock(revenue_mtx);
        cout << "Total revenue:    " << fixed << setprecision(2) << final_total_revenue << "\n";
    }

    //список перших кількох результ
    {
        lock_guard<mutex> lock(results_mtx);
        cout << "Stored results:   " << processed_results.size() << "\n";
        cout << "\nSample (up to 5 rows):\n";
        cout << "order_id | qty | price  | revenue | score\n";
        cout << "-----------------------------------------\n";
        for (size_t i = 0; i < processed_results.size() && i < 5; ++i) 
    {
            const auto& r = processed_results[i];
            cout << setw(8) << r.order_id << " | "
                 << setw(3) << r.quantity << " | "
                 << setw(6) << fixed << setprecision(2) << r.price << " | "
                 << setw(7) << fixed << setprecision(2) << r.revenue << " | "
                 << fixed << setprecision(6) << r.expensive_score << "\n";
        }
    }

    cout << "\n END \n";
    return 0;
}
