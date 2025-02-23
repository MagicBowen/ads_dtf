#include <iostream>
#include <vector>
#include <memory>
#include <future>
#include <atomic>
#include <unordered_map>
#include <any>
#include <typeindex>
#include <optional>
#include <mutex>
#include <chrono>

enum class Status {
    OK,
    CANCELLED,
    ERROR
};

// 数据上下文，支持类型安全的数据存取
class DataContext {
    std::unordered_map<std::type_index, std::any> data_map_;
    std::size_t data_index_ = 0;
    mutable std::mutex mutex_;

public:
    template<typename T>
    void set(const T& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_map_[typeid(T)] = data;
    }

    template<typename T>
    std::optional<T> get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_map_.find(typeid(T));
        if (it != data_map_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    void set_data_index(std::size_t index) { data_index_ = index; }
    std::size_t get_data_index() const { return data_index_; }
};

// 算法基类
class Algorithm {
public:
    virtual ~Algorithm() = default;
    virtual void Init() {}
    virtual Status Exec(DataContext& ctx) = 0;
};

// 执行器接口
class Executor {
public:
    virtual ~Executor() = default;
    virtual Status execute(DataContext& ctx, const std::atomic<bool>* stop_flag = nullptr) = 0;
};

// 基础执行器包装算法
class AlgorithmExecutor : public Executor {
    std::shared_ptr<Algorithm> algo_;
public:
    AlgorithmExecutor(std::shared_ptr<Algorithm> algo) : algo_(algo) {}

    Status execute(DataContext& ctx, const std::atomic<bool>* stop_flag) override {
        if (stop_flag && stop_flag->load()) {
            return Status::CANCELLED;
        }
        return algo_->Exec(ctx);
    }
};

// 串行执行器
class SequentialExecutor : public Executor {
    std::vector<std::unique_ptr<Executor>> children_;

public:
    SequentialExecutor(std::vector<std::unique_ptr<Executor>> children)
        : children_(std::move(children)) {}

    Status execute(DataContext& ctx, const std::atomic<bool>* stop_flag) override {
        for (auto& child : children_) {
            if (stop_flag && stop_flag->load()) {
                return Status::CANCELLED;
            }
            Status status = child->execute(ctx, stop_flag);
            if (status != Status::OK) {
                return status;
            }
        }
        return Status::OK;
    }
};

// 并行执行器
class ParallelExecutor : public Executor {
    std::vector<std::unique_ptr<Executor>> children_;

public:
    ParallelExecutor(std::vector<std::unique_ptr<Executor>> children)
        : children_(std::move(children)) {}

    Status execute(DataContext& ctx, const std::atomic<bool>* stop_flag) override {
        std::vector<std::future<Status>> futures;
        // std::vector<DataContext> contexts;

        // for (size_t i = 0; i < children_.size(); ++i) {
        //     contexts.emplace_back(ctx);
        //     contexts.back().set_data_index(i);
        // }

        for (size_t i = 0; i < children_.size(); ++i) {
            futures.emplace_back(std::async(std::launch::async, 
                [&, i]() { return children_[i]->execute(ctx, stop_flag); }));
        }

        Status overall = Status::OK;
        for (auto& fut : futures) {
            Status s = fut.get();
            if (s != Status::OK) {
                overall = s;
            }
        }
        return overall;
    }
};

// 竞争执行器
class RaceExecutor : public Executor {
    std::vector<std::unique_ptr<Executor>> children_;

public:
    RaceExecutor(std::vector<std::unique_ptr<Executor>> children)
        : children_(std::move(children)) {}

    Status execute(DataContext& ctx, const std::atomic<bool>* parent_stop_flag) override {
        std::atomic<bool> stop_flag(false);
        std::vector<std::future<Status>> futures;
        std::promise<Status> result_promise;
        auto result_future = result_promise.get_future();

        for (auto& child : children_) {
            futures.emplace_back(std::async(std::launch::async, [&]() {
                // DataContext child_ctx = ctx;
                Status status = child->execute(ctx, &stop_flag);
                if (status == Status::OK) {
                    bool expected = false;
                    if (stop_flag.compare_exchange_strong(expected, true)) {
                        result_promise.set_value(status);
                    }
                }
                return status;
            }));
        }

        Status result = result_future.get();
        stop_flag.store(true);

        for (auto& fut : futures) {
            fut.wait();
        }

        return result;
    }
};

// 数据并行执行器
class DataParallelExecutor : public Executor {
    std::unique_ptr<Executor> child_;

public:
    DataParallelExecutor(std::unique_ptr<Executor> child)
        : child_(std::move(child)) {}

    Status execute(DataContext& ctx, const std::atomic<bool>* stop_flag) override {
        auto data_objs = ctx.get<std::vector<int>>();
        if (!data_objs) return Status::ERROR;

        std::vector<std::future<Status>> futures;
        for (size_t i = 0; i < data_objs->size(); ++i) {
            futures.emplace_back(std::async(std::launch::async, [&, i]() {
                // DataContext child_ctx = ctx;
                // child_ctx.set_data_index(i);
                return child_->execute(ctx, stop_flag);
            }));
        }

        Status overall = Status::OK;
        for (auto& fut : futures) {
            Status s = fut.get();
            if (s != Status::OK) {
                overall = s;
            }
        }
        return overall;
    }
};

// 测试用算法实现
class Algo1 : public Algorithm {
public:
    Status Exec(DataContext& ctx) override {
        auto index = ctx.get_data_index();
        std::cout << "Algo1 processing data index: " << index << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return Status::OK;
    }
};

class Algo2 : public Algorithm {
public:
    Status Exec(DataContext& ctx) override {
        auto index = ctx.get_data_index();
        std::cout << "Algo2 processing data index: " << index << std::endl;
        return Status::OK;
    }
};

class Algo3 : public Algorithm {
public:
    Status Exec(DataContext& ctx) override {
        std::cout << "Algo3 executing" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return Status::OK;
    }
};

class Algo4 : public Algorithm {
public:
    Status Exec(DataContext& ctx) override {
        std::cout << "Algo4 executing" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        return Status::OK;
    }
};

int main() {
    // 构造测试用算法
    auto algo1 = std::make_shared<Algo1>();
    auto algo2 = std::make_shared<Algo2>();
    auto algo3 = std::make_shared<Algo3>();
    auto algo4 = std::make_shared<Algo4>();

    std::vector<std::unique_ptr<Executor>> algos;
    algos.emplace_back(std::make_unique<AlgorithmExecutor>(algo1));
    algos.emplace_back(std::make_unique<AlgorithmExecutor>(algo2));

    // 构建执行流程：data_parallel(sequential(Algo1, Algo2))
    auto data_parallel_flow = std::make_unique<DataParallelExecutor>(
        std::make_unique<SequentialExecutor>(std::move(algos)));

    std::vector<std::unique_ptr<Executor>> algos1;
    algos1.emplace_back(std::move(data_parallel_flow));
    algos1.emplace_back(std::make_unique<AlgorithmExecutor>(algo4));

    std::vector<std::unique_ptr<Executor>> algos2;
    algos2.emplace_back(std::make_unique<AlgorithmExecutor>(algo3));
    algos2.emplace_back(std::make_unique<RaceExecutor>(std::move(algos1)));

    // 构建主流程：sequential(Algo3 -> race(data_parallel_flow, Algo4))
    auto main_flow = std::make_unique<SequentialExecutor>(std::move(algos2));

    // 准备测试数据
    DataContext ctx;
    ctx.set(std::vector<int>{0, 1, 2}); // 数据并行使用3个数据实例

    std::atomic<bool> stop_flag(false);

    // 执行流程
    Status result = main_flow->execute(ctx, &stop_flag);

    std::cout << "Final result: " << static_cast<int>(result) << std::endl;
    return 0;
}