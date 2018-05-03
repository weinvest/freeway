//
// Created by shugan.li on 18-3-23.
//

#ifndef FREEWAY_TASKLIST_H
#define FREEWAY_TASKLIST_H

class Task;
class TaskList {

public:
    TaskList();
    TaskList(const TaskList&) = delete;

    TaskList(TaskList&& o);

    ~TaskList();

    void PushFront(Task* pTask);
    void PushBack(Task* pTask);

    Task* PopFront( void );
    Task* PopBack( void );

    static void Erase(Task* pTask);

    void InsertAfter(Task* pPrev, Task* pTask);
    void InsertBefore(Task* pNext, Task* pTask);

    bool Empty( void ) const;

    Task* Front( void ) const;
    Task* Back( void );

    bool TraverseEnd(Task *pTask) const { return mHead == pTask; }

private:
    Task* mHead{nullptr};
};


#endif //FREEWAY_TASKLIST_H
