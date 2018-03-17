#ifndef _SPSC_QUEUE_HPP_
#define _SPSC_QUEUE_HPP_

#include <cassert>
template<typename T>
struct spsc_queue
{
    T head;
    char _pading[64-sizeof(T)];
    T tail;

    spsc_queue(const spsc_queue&) = delete;

    spsc_queue( void )
            : head(nullptr)
            , tail(nullptr)
    {
    }

    void init(T first)
    {
        head = tail = first;
        first->next = nullptr;
        assert(nullptr != head);
    }

    [[gnu::noinline]]
    auto pop(T* last)
    {
        auto v = head;
        auto n = v->next;
        if(nullptr == n)
        {
            if(last != nullptr)
                *last = v;
            return n;
        }

        head = n;
        v->next = nullptr;
        return v;
    }

    [[gnu::noinline]]
    auto pop()
    {
        auto v = head;
        auto n = v->next;
        if(nullptr == n)
        {
            return n;
        }

        head = n;
        v->next = nullptr;
        return v;
    }

    auto first()
    {
        return head;
    }

    auto next(T v)
    {
        assert(nullptr != v);
        auto n = v->next;
        return n;
    }

    auto pop2(T v)
    {
        head = v;
    }

    bool empty() { return nullptr == head->next; }

    void push(T v)
    {
        assert(nullptr != v && nullptr == v->next);
        tail->next = v;
        tail = v;
        tail->next = nullptr;
    }

};
#endif
