//
// Created by shugan.li on 18-3-23.
//

#ifndef FREEWAY_TASKLIST_H
#define FREEWAY_TASKLIST_H

class Task;
class TaskList {

public:
    TaskList() = default;
    TaskList(const TaskList&) = delete;

    TaskList(TaskList&& o);

    Task* Pop( void );
    void Push(Task* pTask);

    bool Empty( void ) const;
private:
    Task* mHead{nullptr};
};


#endif //FREEWAY_TASKLIST_H