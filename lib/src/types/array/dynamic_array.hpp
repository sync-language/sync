//! API
#ifndef SY_TYPES_ARRAY_DYNAMIC_ARRAY_HPP_
#define SY_TYPES_ARRAY_DYNAMIC_ARRAY_HPP_

#include "../../core.h"
#include <utility>
#include "../../mem/allocator.hpp"

namespace sy {

    class Type;

    class SY_API RawDynArrayUnmanaged SY_CLASS_FINAL {
    public:

        RawDynArrayUnmanaged() = default;

        ~RawDynArrayUnmanaged() noexcept;

        void destroy(Allocator& alloc, void (*destruct)(void *ptr), size_t size, size_t align) noexcept;

        void destroyScript(Allocator& alloc, const Type* typeInfo) noexcept;

        RawDynArrayUnmanaged(RawDynArrayUnmanaged&& other) noexcept;

        RawDynArrayUnmanaged& operator=(RawDynArrayUnmanaged&& other) = delete;

        void moveAssign(
            RawDynArrayUnmanaged&& other,
            void (*destruct)(void *ptr),
            Allocator& alloc,
            size_t size,
            size_t align
        ) noexcept;

        RawDynArrayUnmanaged(const RawDynArrayUnmanaged& other) = delete;

        [[nodiscard]] static AllocExpect<RawDynArrayUnmanaged> copyConstruct(
            const RawDynArrayUnmanaged& other,
            Allocator& alloc,
            void (*copyConstructFn)(void* dst, const void* src),
            size_t size,
            size_t align
        ) noexcept;

        // TODO script types copy
        // [[nodiscard]] static AllocExpect<RawDynArrayUnmanaged> copyConstructScript(
        //     const RawDynArrayUnmanaged& other,
        //     Allocator& alloc,
        //     const Type* typeInfo
        // ) noexcept;

        RawDynArrayUnmanaged& operator=(const RawDynArrayUnmanaged& other) = delete;

        [[nodiscard]] AllocExpect<void> copyAssign(
            const RawDynArrayUnmanaged& other,
            Allocator& alloc,
            void (*destruct)(void *ptr),
            void (*copyConstructFn)(void* dst, const void* src),
            size_t size,
            size_t align
        ) noexcept;

        // TODO script types copy
        // [[nodiscard]] AllocExpect<void> copyAssignScript(
        //     const DynArrayUnmanaged& other,
        //     Allocator& alloc,
        //     const Type* typeInfo
        // ) noexcept;

        [[nodiscard]] size_t len() const { return len_; }

        [[nodiscard]] const void* at(size_t index, size_t size) const;

        [[nodiscard]] void* at(size_t index, size_t size);

        [[nodiscard]] const void* data() const { return this->data_; }

        [[nodiscard]] void* data() { return this->data_; }

        [[nodiscard]] AllocExpect<void> pushBack(
            void* element,
            Allocator& alloc,
            size_t size,
            size_t align
        ) noexcept;

        [[nodiscard]] AllocExpect<void> pushBackCustomMove(
            void* element,
            Allocator& alloc,
            size_t size,
            size_t align,
            void(*moveConstructFn)(void* dst, void* src)
        ) noexcept;

        [[nodiscard]] AllocExpect<void> pushBackScript(
            void* element,
            Allocator& alloc,
            const Type* typeInfo
        ) noexcept;

        [[nodiscard]] AllocExpect<void> pushFront(
            void* element,
            Allocator& alloc, 
            size_t size, 
            size_t align
        ) noexcept;

        [[nodiscard]] AllocExpect<void> pushFrontCustomMove(
            void* element,
            Allocator& alloc,
            size_t size,
            size_t align,
            void(*moveConstructFn)(void* dst, void* src)
        ) noexcept;

        [[nodiscard]] AllocExpect<void> pushFrontScript(
            void* element,
            Allocator& alloc,
            const Type* typeInfo
        ) noexcept;

    private:

        [[nodiscard]] AllocExpect<void> reallocateBack(
            Allocator& alloc,
            const size_t size,
            const size_t align
        ) noexcept;

        [[nodiscard]] AllocExpect<void> reallocateBackCustomMove(
            Allocator& alloc,
            const size_t size,
            const size_t align,
            void(*moveConstructFn)(void* dst, void* src)
        ) noexcept;

        [[nodiscard]] AllocExpect<void> reallocateFront(
            Allocator& alloc,
            const size_t size,
            const size_t align
        ) noexcept;

        [[nodiscard]] AllocExpect<void> reallocateFrontCustomMove(
            Allocator& alloc,
            const size_t size,
            const size_t align,
            void(*moveConstructFn)(void* dst, void* src)
        ) noexcept;

        [[nodiscard]] void* beforeFront(const size_t size);

    private:
        size_t  len_ = 0;
        void*   data_ = nullptr;
        size_t  capacity_ = 0;
        void*   alloc_ = nullptr;
    };


    
    /// Dynamically resizable array. Grows to fit elements you push into it.
    template <typename T>
    class DynArray {
        T*      _data =nullptr;
        size_t  _length =0;
        size_t  _capacity =0;
    public:
        DynArray()=default;
        ~DynArray(){
            if(_data == nullptr){
                return;
            }
            for(size_t i=0; i<_length; i++){
                _data[i].~T();
            }
            delete[] _data;
            _data = nullptr;
        };
        void push(const T& v){
            if(_length == _capacity){
                resize();
            }
            _data[_length] = v;
            _length++;
        }
        void push(T&& v){
            //object has pointer and type t is another dynarray instance
            if(_length == _capacity){
                resize();
            }
            _data[_length] = std::move(v);
            _length++;
        };
        const T& operator[](const size_t i) const{
            //return data indiv
            return _data[i];
        };
        T& operator[](const size_t i){
            return _data[i];
        };

        size_t len() const { return _length; }

    private:
        void resize(){
            const size_t newCapacity = calculateNewCapacity(_capacity);
            T* temp = new T[newCapacity];
            for(size_t i = 0; i<_length; i++){
                temp[i] = _data[i];
            }
            delete[] _data;
            _data = temp;
            _capacity = newCapacity;
        }
        static size_t calculateNewCapacity(size_t currentCapacity) {
            if(currentCapacity == 0){
                currentCapacity = 1;
            }
            currentCapacity *= 2;
            return currentCapacity;
        }
    };
    
};


#endif // SY_TYPES_ARRAY_DYNAMIC_ARRAY_HPP_