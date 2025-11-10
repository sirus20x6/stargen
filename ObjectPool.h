#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <vector>
#include <mutex>
#include <memory>
#include <functional>

/**
 * @brief Thread-safe object pool for reusing expensive-to-create objects
 *
 * Reduces allocation overhead by maintaining a pool of reusable objects.
 * Objects are checked out, used, and then returned to the pool.
 *
 * @tparam T Type of object to pool
 */
template<typename T>
class ObjectPool {
public:
    /**
     * @brief Construct object pool with initial capacity
     * @param initial_size Number of objects to pre-allocate
     * @param factory Function to create new objects when pool is empty
     */
    ObjectPool(size_t initial_size, std::function<T*()> factory)
        : factory_(factory), total_created_(initial_size) {
        pool_.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i) {
            pool_.push_back(factory_());
        }
    }

    /**
     * @brief Destructor - cleans up all pooled objects
     */
    ~ObjectPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (T* obj : pool_) {
            delete obj;
        }
        pool_.clear();
    }

    /**
     * @brief Acquire an object from the pool
     *
     * If pool is empty, creates a new object using the factory.
     * Caller is responsible for returning the object via release().
     *
     * @return Pointer to object (never null)
     */
    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (pool_.empty()) {
            // Pool exhausted - create new object
            total_created_++;
            return factory_();
        }

        // Return object from pool
        T* obj = pool_.back();
        pool_.pop_back();
        return obj;
    }

    /**
     * @brief Return an object to the pool for reuse
     *
     * Object should be reset to a clean state before returning.
     *
     * @param obj Object to return (must not be null)
     */
    void release(T* obj) {
        if (obj == nullptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(obj);
    }

    /**
     * @brief Get current pool size (available objects)
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    /**
     * @brief Get total objects created (initial + dynamically allocated)
     */
    size_t getTotalCreated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_created_;
    }

    /**
     * @brief RAII wrapper for automatic acquire/release
     *
     * Usage:
     *   auto obj = pool.acquireScoped();
     *   obj->doSomething();
     *   // Automatically released when obj goes out of scope
     */
    class ScopedObject {
    public:
        ScopedObject(ObjectPool<T>* pool, T* obj)
            : pool_(pool), obj_(obj) {}

        ~ScopedObject() {
            if (obj_ && pool_) {
                pool_->release(obj_);
            }
        }

        // Non-copyable
        ScopedObject(const ScopedObject&) = delete;
        ScopedObject& operator=(const ScopedObject&) = delete;

        // Movable
        ScopedObject(ScopedObject&& other) noexcept
            : pool_(other.pool_), obj_(other.obj_) {
            other.pool_ = nullptr;
            other.obj_ = nullptr;
        }

        T* get() { return obj_; }
        T* operator->() { return obj_; }
        T& operator*() { return *obj_; }

    private:
        ObjectPool<T>* pool_;
        T* obj_;
    };

    /**
     * @brief Acquire object with RAII wrapper
     */
    ScopedObject acquireScoped() {
        return ScopedObject(this, acquire());
    }

private:
    std::vector<T*> pool_;
    mutable std::mutex mutex_;
    std::function<T*()> factory_;
    size_t total_created_ = 0;  // Track total objects created (for statistics)
};

#endif // OBJECT_POOL_H
