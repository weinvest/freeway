//
// Created by shgli on 17-10-13.
//

#ifndef ARAGOPROJECT_LOCKPTR_H
#define ARAGOPROJECT_LOCKPTR_H

class Task;
class DEventNode;
class DEventNodeSpecial;
class LockPtrBase
{
public:
    LockPtrBase(DEventNode* pNode, DEventNodeSpecial* pSpecial)
            :mNode(pNode)
            ,mSpecial(pSpecial)
    {}

    ~LockPtrBase() { mNode = nullptr; }

    void WaitSharedLock( void );

    bool HasSpecial( void ) const { return nullptr != mSpecial; }

    void reset(const LockPtrBase& o)
    {
        mNode = o.mNode;
        mSpecial = o.mSpecial;
    }

protected:
    DEventNode* mNode{nullptr};
    DEventNodeSpecial* mSpecial{nullptr};
};

template <typename T>
class LockPtr: public LockPtrBase
{
public:
    LockPtr( void )
         :LockPtrBase(nullptr, nullptr)
    {}

    LockPtr(const LockPtrBase& o)
    :LockPtrBase(o)
    {
    }

    T* operator-> ()
    {
        if(!HasSpecial()){
            return (T*)mNode;
        }

        WaitSharedLock();
        return (T*)mNode;
    }

    const T*operator->() const
    {
        return const_cast<LockPtr<T>*>(this)->operator->();
    }

    T* get() { return (T*)mNode; }

    const T* get() const { return (T*)mNode; }

};


#endif //ARAGOPROJECT_LOCKPTR_H
