#include "tuning.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <math.h>
#include <sstream>
#include <vector>

namespace dx7 {

struct StandardTuning : public TuningState {
    StandardTuning() {
        const int base = 50857777;  // (1 << 24) * (log(440) / log(2) - 69/12) 
        const int step = (1 << 24) / 12;
        for( int mn = 0; mn < 128; ++mn )
        {
            auto res = base + step * mn;
            current_logfreq_table_[mn] = res;
        }
    }
    
    virtual int32_t midinote_to_logfreq(int midinote) override {
        return current_logfreq_table_[midinote];
    }
    
    int current_logfreq_table_[128];
};


std::shared_ptr<TuningState> createStandardTuning()
{
    return std::make_shared<StandardTuning>();
}

}