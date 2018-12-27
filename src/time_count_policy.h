
#ifndef __TIME_COUNT_POLICY_H__
#define __TIME_COUNT_POLICY_H__

#include <time.h>

class TimeCountPolicy
{
public:
    TimeCountPolicy(int time_period, int max_count);
    TimeCountPolicy();
    
    void init(int time_period, int max_count);

    int  increase_count(int num);
    int  increase_count();

    int  get_time_period() const
    {
        return time_period_;
    }

    int  get_max_count() const
    {
        return max_count_;
    }

    time_t  get_begin_timestamp() const
    {
        return begin_timestamp_;
    }

    int get_current_count() const
    {
        return current_count_;
    }

private:
    int       time_period_;
    int       max_count_;

    time_t    begin_timestamp_;
    int       current_count_;
};

#endif
