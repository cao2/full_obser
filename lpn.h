
#ifndef _LPN_H_
#define _LPN_H_

#include <assert.h>
#include <vector>
#include "msg_def.h"


class lpn_t
{
private:
    string name;
    std::vector<message_t> msg_vector;
    config_t init_cfg;
    uint32_t index;

public:
    lpn_t() {}
    
    void set_flow_name(const string& n) {
        name = n;
    }
    
    string get_flow_name() {
        return name;
    }
    
    void insert_msg(const message_t& msg) {
        msg_vector.push_back(msg);
        init_cfg = null_cfg;
    }
    
    void set_init_cfg(const config_t c) {
        init_cfg = c;
    }
    
    void set_index(uint32_t idx) {
        index = idx;
    }
    
    uint32_t get_index() {
        return index;
    }
    
    config_t accept(const message_t& other) const {
            const message_t& msg = msg_vector.at(0);
            if (msg.src == other.src && msg.dest == other.dest && msg.cmd == other.cmd && ((init_cfg & msg.pre_cfg) == msg.pre_cfg))
                return msg.post_cfg;
                return null_cfg;
    }

    
    config_t accept(const message_t& other, const config_t& cfg) const {
        for (unsigned i=0; i<msg_vector.size(); i++) {
            const message_t& msg = msg_vector.at(i);
 
            if (msg.src == other.src && msg.dest == other.dest && msg.cmd == other.cmd  && ((cfg & msg.pre_cfg) == msg.pre_cfg)) {
                if ((cfg & ~msg.pre_cfg & msg.post_cfg) != 0) {
                    cout << bitset<12>(cfg) << endl
                        << bitset<12>(msg.pre_cfg) << endl
                    << bitset<12>(msg.post_cfg) << endl;
                    assert(false);
                }
               
                return (cfg & ~msg.pre_cfg) | msg.post_cfg;
            }
        }
        return null_cfg;
    }
};


#endif

