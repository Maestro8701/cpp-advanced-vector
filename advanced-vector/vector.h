/* разместите свой код в этом файле */

#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(other.buffer_)
        , capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Deallocate(buffer_);
            buffer_ = rhs.buffer_;
            capacity_ = rhs.capacity_;
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }
        return *this;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;       

template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end());
        size_t position = pos - cbegin();

        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            T* new_element = new (new_data + position) T(std::forward<Args>(args)...);
            try {
                CopyOrMoveData(new_data, 0, position, 0);
                CopyOrMoveData(new_data, position, size_ - position, position + 1);
            }
            catch (...) {
                std::destroy_at(new_element);
                std::destroy_n(data_.GetAddress(), size_);
                throw;
            }
            ReplacingOldMemory(new_data);
        }
        else {
            if (pos == cend()) {
                new (end()) T(std::forward<Args>(args)...);
            }
            else {
                T temp(std::forward<Args>(args)...);
                new (end()) T(std::move(*(end() - 1)));
                try {
                    std::move_backward(begin() + position, end() - 1, end());
                }
                catch (...) {
                    std::destroy_at(end());
                    throw;
                }
                data_[position] = std::move(temp);
            }
        }
        ++size_;
        return begin() + position;
    }    


iterator begin() noexcept {
    return data_.GetAddress();
}

iterator end() noexcept {
    return data_.GetAddress() + size_;
}

const_iterator begin() const noexcept {
    return data_.GetAddress();
}

const_iterator end() const noexcept {
    return data_.GetAddress() + size_;
}

const_iterator cbegin() const noexcept {
    return begin();
}

const_iterator cend() const noexcept {
    return end();
}

iterator Insert(const_iterator pos, const T& value) {
    assert(pos >= begin() && pos <= end());
    size_t position = pos - cbegin();

    if (size_ == Capacity()) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        T* new_element = new (new_data + position) T(value);
        try {
            CopyOrMoveData(new_data, 0, position, 0);
            CopyOrMoveData(new_data, position, size_ - position, position + 1);
        }
        catch (...) {
            std::destroy_at(new_element);
            std::destroy_n(data_.GetAddress(), size_);
            throw;
        }
        ReplacingOldMemory(new_data);
    }
    else {
        if (pos == cend()) {
            new (end()) T(value);
        }
        else {
            T temp(value);
            new (end()) T(std::move(*(end() - 1)));
            try {
                std::move_backward(begin() + position, end() - 1, end());
            }
            catch (...) {
                std::destroy_at(end());
                throw;
            }
            data_[position] = std::move(temp);
        }
    }
    ++size_;
    return begin() + position;
}

iterator Insert(const_iterator pos, T&& value) {
    assert(pos >= begin() && pos <= end());
    size_t position = pos - cbegin();

    if (size_ == Capacity()) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        T* new_element = new (new_data + position) T(std::move(value));
        try {
            CopyOrMoveData(new_data, 0, position, 0);
            CopyOrMoveData(new_data, position, size_ - position, position + 1);
        }
        catch (...) {
            std::destroy_at(new_element);
            std::destroy_n(data_.GetAddress(), size_);
            throw;
        }
        ReplacingOldMemory(new_data);
    }
    else {
        if (pos == cend()) {
            new (end()) T(std::move(value));
        }
        else {
            T temp(std::move(value));
            new (end()) T(std::move(*(end() - 1)));
            try {
                std::move_backward(begin() + position, end() - 1, end());
            }
            catch (...) {
                std::destroy_at(end());
                throw;
            }
            data_[position] = std::move(temp);
        }
    }
    ++size_;
    return begin() + position;
}

iterator Erase(const_iterator pos) {
    assert(pos >= begin() && pos < end());
    size_t position = pos - cbegin();

    std::destroy_at(data_ + position);

    if (pos != cend() - 1) {
        std::move(begin() + position + 1, end(), begin() + position);
    }

    --size_;
    return begin() + position;
}
    
    Vector() = default;

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        } else if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    } 
    
void PushBack(const T& value){
    if (size_ == Capacity()) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        std::uninitialized_copy_n(&value, 1, new_data.GetAddress() + size_);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else {
        new (data_ + size_) T(value);
    }
    ++size_; 
}

void PushBack(T&& value){
    if (size_ == Capacity()) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else {
        new (data_ + size_) T(std::move(value));
    }
    ++size_;
}
    
template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        T* result = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data + size_) T(std::forward<Args>(args)...);
            try {
                CopyOrMoveData(new_data, 0, size_, 0);
            }
            catch (...) {
                std::destroy_n(new_data.GetAddress() + size_, 1);
                throw;           
            }
            ReplacingOldMemory(new_data);
        }
        else {
            result = new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *result;
    }
    
    void CopyOrMoveData(RawMemory<T>& new_data, size_t from, size_t quantity, size_t dest_from) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress() + from, quantity, new_data.GetAddress() + dest_from);
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress() + from, quantity, new_data.GetAddress() + dest_from);
        }
    }

    void ReplacingOldMemory(RawMemory<T>& new_data) {
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    
    void PopBack() noexcept {
        if (size_ > 0) {
            std::destroy_at(data_ + size_ - 1);
            --size_;
        }
    }
    
    explicit Vector(size_t size)
        : data_(size)
        , size_(size) {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_) {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_) {
        other.size_ = 0;
    }

Vector& operator=(const Vector& rhs) {
    if (this != &rhs) {
        if (rhs.size_ > data_.Capacity()) {
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
        } else {
            if (size_ > rhs.size_) {
                std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
            } else {
                std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
            }
            std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + std::min(size_, rhs.size_), data_.GetAddress());
            size_ = rhs.size_;
        }
    }
    return *this;
}

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            data_ = std::move(rhs.data_);
            size_ = rhs.size_;
            rhs.size_ = 0;
        }
        return *this;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }
    

    
    
private:
    RawMemory<T> data_;
    size_t size_ = 0;
};    



