#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

class Account {
    std::string id;
    double balance;
public:
    Account(std::string id, double balance): id(std::move(id)), balance(balance) {}
    const std::string& getId() const { return id; }
    double getBalance() const { return balance; }
    friend class TransactionManager;
};

enum class TransactionType { Deposit, Withdraw };

class Transaction {
    int id;
    std::string accountId;
    TransactionType type;
    double amount;
    bool success;
public:
    Transaction(int id, std::string accountId, TransactionType type, double amount, bool success)
        : id(id), accountId(std::move(accountId)), type(type), amount(amount), success(success) {}

    friend std::ostream& operator<<(std::ostream& os, const Transaction& t) {
        os << "#" << t.id << " [" << t.accountId << "] "
           << (t.type == TransactionType::Deposit ? "DEPOSIT " : "WITHDRAW ")
           << t.amount << (t.success ? " OK" : " RECHAZADA");
        return os;
    }
};

class TransactionManager {
    std::mutex mtx;
    std::vector<Transaction> history;
    int nextId = 1;

    TransactionManager() = default;

    void record(Account* acc, TransactionType type, double amount, bool ok) {
        history.emplace_back(nextId++, acc->id, type, amount, ok);
    }

public:
    TransactionManager(const TransactionManager&) = delete;
    TransactionManager& operator=(const TransactionManager&) = delete;
    TransactionManager(TransactionManager&&) = delete;
    TransactionManager& operator=(TransactionManager&&) = delete;

    // C++11 garantiza inicialización thread-safe de estáticos locales
    static TransactionManager& getInstance() {
        static TransactionManager instance;
        return instance;
    }

    bool deposit(Account* acc, double amount) {
        std::lock_guard<std::mutex> lock(mtx);
        bool ok = acc != nullptr && amount > 0;
        if (ok) acc->balance += amount;
        if (acc) record(acc, TransactionType::Deposit, amount, ok);
        return ok;
    }

    bool withdraw(Account* acc, double amount) {
        std::lock_guard<std::mutex> lock(mtx);
        bool ok = acc != nullptr && amount > 0 && acc->balance >= amount;
        if (ok) acc->balance -= amount;
        if (acc) record(acc, TransactionType::Withdraw, amount, ok);
        return ok;
    }

    std::size_t transactionCount() {
        std::lock_guard<std::mutex> lock(mtx);
        return history.size();
    }

    void printHistory() {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& t : history) std::cout << "  " << t << '\n';
    }
};

int main() {
    auto& manager = TransactionManager::getInstance();

    Account account("A1", 1000);

    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            manager.deposit(&account, 100);
            manager.withdraw(&account, 50);
        });
    }

    for (auto& t : threads) t.join();

    std::cout << "Balance final: "
              << account.getBalance()
              << std::endl;

    std::cout << "Direccion manager: "
              << &TransactionManager::getInstance()
              << std::endl;

    // Prueba de estrés: 100 hilos x 1000 operaciones
    Account stress("A2", 0);
    std::vector<std::thread> pool;
    for (int i = 0; i < 100; ++i) {
        pool.emplace_back([&]() {
            auto& m = TransactionManager::getInstance();
            for (int j = 0; j < 1000; ++j) {
                m.deposit(&stress, 2);
                m.withdraw(&stress, 1);
            }
        });
    }
    for (auto& t : pool) t.join();

    std::cout << "\nStress A2 (esperado 100000): " << stress.getBalance() << std::endl;

    // Retiro sin fondos suficiente
    Account pobre("A3", 10);
    bool ok = manager.withdraw(&pobre, 50);
    std::cout << "Retiro de 50 en A3 (saldo 10): " << (ok ? "OK" : "RECHAZADO")
              << ", balance: " << pobre.getBalance() << std::endl;

    std::cout << "Total transacciones registradas: " << manager.transactionCount() << std::endl;
    std::cout << "Mismo manager desde otro hilo: ";
    std::thread([]() { std::cout << &TransactionManager::getInstance() << std::endl; }).join();
}
