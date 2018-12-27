
#include "time_count_policy.h"
#include "log.h"

TimeCountPolicy::TimeCountPolicy(int time_period, int max_count)
:time_period_(time_period),
 max_count_(max_count),
 begin_timestamp_(time(0)),
 current_count_(0)
{
}

TimeCountPolicy::TimeCountPolicy()
{
}

void
TimeCountPolicy::init(int time_period, int max_count)
{
    time_period_ = time_period;
    max_count_ = max_count;
    begin_timestamp_ = time(0);
    current_count_ = 0;
}

int
TimeCountPolicy::increase_count(int num)
{
    int ret = 0;
    time_t now = time(0);

    trpc_debug_log("max_count=%d, current_count_=%d, now - begin_timestamp_ = %ld, time_period_=%d",
                    max_count_, current_count_, now - begin_timestamp_ , time_period_);
    if (current_count_ == 0
        ||(now - begin_timestamp_ >= time_period_))
    {
        begin_timestamp_ = now;
        current_count_ = num;
    }
    else
    {
        current_count_ += num;
        if (current_count_ > max_count_)
        {
            ret = -1;
        }
    }

    trpc_debug_log("max_count_=%d, current_count_=%d, now - begin_timestamp_ = %ld, time_period_=%d, ret = %d",
                    max_count_, current_count_, now - begin_timestamp_ , time_period_, ret);

    return ret;
}


int
TimeCountPolicy::increase_count()
{
    return increase_count(1);
}
