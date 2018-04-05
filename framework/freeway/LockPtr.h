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
    ~LockPtrBase() { mNode = nullptr; }

    void Connect(DEventNode* pSuccessor);

    void WaitSharedLock( void );
    bool HasSharedLock4(Task* pTask) const;
    bool HasSpecial( void ) const { return nullptr != mSpecial; }

protected:
    LockPtrBase(DEventNode* pNode)
            :mNode(pNode)
    {}

    DEventNodeSpecial* mSpecial{nullptr};
    DEventNode* mNode{nullptr};
};

template <typename T>
class LockPtr: public LockPtrBase
{
public:
    LockPtr(T* pNode)
         :LockPtrBase(pNode)
    {}

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
