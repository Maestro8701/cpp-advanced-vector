#include <cassert>
#include <iterator>
#include <utility>
#include <memory>
#include <type_traits>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity)
    {
    }

    RawMemory(const RawMemory&) = delete;

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        :buffer_(std::exchange(other.buffer_, nullptr)),
        capacity_(std::exchange(other.capacity_, 0))
    {
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::exchange(rhs.buffer_, nullptr);
        capacity_ = std::exchange(rhs.capacity_, 0);
        return *this;
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

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(begin(), size);
    }

    explicit Vector(const Vector& other)
        : data_(other.Size())
        , size_(other.Size())
    {
        std::uninitialized_copy_n(other.begin(), other.Size(), begin());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(std::exchange(other.size_, 0))
    {
    }

    Vector& operator=(const Vector& rhs) {
        if (rhs.Size() > Capacity()) {
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
        }
        else {
            if (rhs.Size() < Size()) {
                for (size_t i = 0; i < rhs.Size(); ++i) {
                    data_[i] = rhs[i];
                }
                std::destroy_n(begin() + rhs.Size(), Size() - rhs.Size());
            }
            else {
                for (size_t i = 0; i < Size(); ++i) {
                    data_[i] = rhs[i];
                }
                std::uninitialized_copy_n(rhs.begin() + Size(),
                    rhs.Size() - Size(), begin() + Size());
            }
            size_ = rhs.Size();
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs);
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Resize(size_t new_size) {
        if (new_size < Size()) {
            std::destroy_n(begin() + new_size, Size() - new_size);
        }
        else if (new_size > Size()) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(begin() + Size(), new_size - Size());
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        Emplace(end(), std::forward<const T&>(value));
    }

    void PushBack(T&& value) {
        Emplace(end(), std::forward<T&&>(value));
    }

    void PopBack() noexcept {
        assert(Size() > 0);
        std::destroy_n(begin() + Size() - 1, 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        return *(Emplace(end(), std::forward<Args>(args)...));
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
        assert(index < Size());
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        MoveDataOrCopyIfDataNonMovable(begin(), Size(), new_data.GetAddress());
        std::destroy_n(begin(), Size());
        data_.Swap(new_data);
    }

    ~Vector() {
        std::destroy_n(begin(), Size());
    }

    iterator begin() noexcept {
        return iterator{ data_.GetAddress() };
    }

    iterator end() noexcept {
        return iterator{ data_.GetAddress() + Size()};
    }

    const_iterator begin() const noexcept {
        return const_iterator{ data_.GetAddress() };
    }

    const_iterator end() const noexcept {
        return const_iterator{ data_.GetAddress() + Size() };
    }

    const_iterator cbegin() const noexcept {
        return const_iterator{ data_.GetAddress() };
    }

    const_iterator cend() const noexcept {
        return const_iterator{ data_.GetAddress() + Size() };
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        if (pos >= begin() && pos <= end()) {
            size_t distance = pos - begin();
            if (Size() == Capacity()) {
                RawMemory<T> new_data(Size() == 0 ? 1 : Size() * 2);
                new (new_data + distance) T(std::forward<Args>(args)...);
                MoveDataOrCopyIfDataNonMovable(begin(), distance, new_data.GetAddress());
                MoveDataOrCopyIfDataNonMovable(begin() + distance,
                                               Size() - distance,
                                               new_data.GetAddress() + distance + 1);
                std::destroy_n(begin(), Size());
                data_.Swap(new_data);
            }
            else if (Size() < Capacity()) {
                if (pos == end()) {
                    new (end()) T(std::forward<Args>(args)...);
                } 
                else {
                    T new_data(std::forward<Args>(args)...);
                    new (end()) T(std::forward<T>(data_[Size() - 1]));
                    std::move_backward(begin() + distance, end() - 1, end());
                    data_[distance] = std::forward<T>(new_data);
                }
            }
            ++size_;
            return begin() + distance;
        }
        else {
            return nullptr;
        }
    }
    
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        if (pos >= begin() && pos <= end()) {
            size_t distance = pos - begin();
            std::move(begin() + distance + 1, end(), begin() + distance);
            std::destroy_n(begin() + Size() - 1, 1);
            --size_;
            return begin() + distance;
        }
        else {
            return nullptr;
        }
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    void MoveDataOrCopyIfDataNonMovable(iterator from, size_t size, iterator to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, size, to);            
        }
        else {
            std::uninitialized_copy_n(from, size, to);
        }
    }
};
