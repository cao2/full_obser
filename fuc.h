#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include "lpn.h"
#include <set>
#include <algorithm>    // std::sort
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>/* needed to define getpid() */
#include <stdio.h>/* needed for printf() */
#include <pthread.h>
#include <sstream>
using namespace std;

void getMemUsage(unsigned int pid, char* name)
{
    printf("my process ID is %d\n", pid);
    
    char cmd[256];
    sprintf(cmd, "top -stats \"pid,command,cpu, mem\" -l 1 -pid %d | grep \"%d\" >> %s", pid, pid,name);
    system(cmd);
}

void max_mem(char *filename){
    
    uint32_t rst=0;
    ifstream trace_file(filename);
    int num;
    if (trace_file.is_open()) {
        std::string line;
        while (getline(trace_file, line)){
            stringstream ss(line);
            string temp;
            ss>>temp;
            ss>>temp;
            ss>>temp;
            ss>>temp;
            if (temp.substr(temp.length()-2).compare("K+")==0){
                num=stoi(temp.substr(0,temp.length()-2),nullptr);
                if (num>rst){
                    rst=num;
                }
            }
            else if (temp.substr(temp.length()-2).compare("M+")==0){
                num=stoi(temp.substr(0,temp.length()-2),nullptr)*1000;
                if (num>rst){
                    rst=num;
                }
            }
            else
                cout<<"erro processing "<<temp<<endl;

        }
        trace_file.close();
        
    }
    else {
        cout << "Unable to open file" << endl;
    }
    cout<<"memory usage : "<<rst<<" KB"<<endl;
    
}

struct flow_instance_t
{
    lpn_t* flow_inst;
    config_t cfg;
    uint32_t addr;
    
    flow_instance_t() {
        flow_inst = nullptr;
        cfg = null_cfg;
    }
    
    flow_instance_t(uint32_t x) {
        flow_inst = nullptr;
        cfg = null_cfg;
        addr = x;
    }
    
    flow_instance_t(const flow_instance_t& other) {
        flow_inst = other.flow_inst;
        cfg = other.cfg;
        addr= other.addr;
    }
    
    bool operator==(const flow_instance_t& other) {
        
        return (flow_inst->get_flow_name() == other.flow_inst->get_flow_name() &&cfg == other.cfg  && addr == other.addr);
    }
    
    flow_instance_t& operator=(const flow_instance_t& other) {
        flow_inst = other.flow_inst;
        cfg = other.cfg;
        addr= other.addr;
        return *this;
    }
    
};

struct active_list{
    vector<uint32_t> rd0,rd1,wt0,wt1,wb0,wb1;
    active_list(){}
    void sortall(){
        sort(rd0.begin(),rd0.end());
        sort(rd1.begin(),rd1.end());
        sort(wt0.begin(),wt0.end());
        sort(wt1.begin(),wt1.end());
        sort(wb0.begin(),wb0.end());
        sort(wb1.begin(),wb1.end());
    }
    
};

struct scenario_t{
    uint32_t read1;
    uint32_t read1_ch;
    uint32_t read0;
    uint32_t read0_ch;
    uint32_t write0;
    uint32_t write1;
    uint32_t write0_ch;
    uint32_t write1_ch;
    uint32_t wb0,wb1;
    vector<flow_instance_t> active_t;
    //active_list active_sort;
    vector<uint32_t> order_finish;
    vector<uint32_t> order_addr;
    scenario_t(){
        read0=0;
        read1=0;
        write0=0;
        write1=0;
        read0_ch=0;
        read1_ch=0;
        write0_ch=0;
        write1_ch=0;
        wb0=0;
        wb1=0;
    }
    
    
};


//typedef vector<flow_instance_t> scenario_t;
uint32_t state(uint32_t cfg){
    for (uint32_t i = 0; i < 32; i++) {
        if ((cfg & 1) == 1 )
            return i;
        cfg = cfg >> 1;
    }
    return 33;
}


std::hash<std::string> str_hash;

active_list sort(vector<flow_instance_t> active_t){
    uint32_t inde;
    active_list sorted;
    
    for(uint32_t i=0;i<active_t.size();i++){
        inde=active_t.at(i).flow_inst->get_index();
        if (inde==0)
            sorted.rd0.push_back(active_t.at(i).cfg);
        else if (inde ==1)
            sorted.rd1.push_back(active_t.at(i).cfg);
        else if (inde ==2)
            sorted.wt0.push_back(active_t.at(i).cfg);
        else if (inde ==3)
            sorted.wt1.push_back(active_t.at(i).cfg);
        else if (inde ==4)
            sorted.wb0.push_back(active_t.at(i).cfg);
        else if (inde ==5)
            sorted.wb1.push_back(active_t.at(i).cfg);
    }
    sorted.sortall();
    return sorted;
    
}

void print_scenario(const vector<lpn_t*> flow_spec, const scenario_t& sce)
{
    vector<flow_instance_t> scen=sce.active_t;
    
    vector<uint32_t> flow_inst_cnt;
    flow_inst_cnt.push_back(sce.read0+sce.read0_ch);
    flow_inst_cnt.push_back(sce.read1+sce.read1_ch);
    flow_inst_cnt.push_back(sce.write0+sce.write0_ch);
    flow_inst_cnt.push_back(sce.write1+sce.write1_ch);
    flow_inst_cnt.push_back(sce.wb0);
    flow_inst_cnt.push_back(sce.wb1);
    //    cout << "order of finishing:"<<endl;
    //    for (uint32_t oin=0; oin<sce.order_addr.size(); oin++) {
    //        uint32_t flw=sce.order_finish.at(oin);
    //        uint32_t adr=sce.order_addr.at(oin);
    //        if (flw==0)
    //            cout<<"read0: ";
    //        else if (flw==1)
    //            cout<<"read1: ";
    //        else if (flw==2)
    //            cout<<"wt0: ";
    //        else if (flw==3)
    //            cout<<"wt1: ";
    //        else if (flw==4)
    //            cout<<"wb0: ";
    //        else if (flw==5)
    //            cout<<"wb1: ";
    //        else if (flw==6)
    //            cout<<"rd0_cache: ";
    //        else if (flw==7)
    //            cout<<"rd1_cache: ";
    //        else if (flw==8)
    //            cout<<"wt0_cache: ";
    //        else if (flw==9)
    //            cout<<"wt1_cache: ";
    //        cout<<adr<<" "<<endl;
    //
    //    }
    cout << "finished flow instances:" << endl;
    cout << "\t cpu0 read: \t"<< sce.read0<<endl;
    cout << "\t cpu0 read active cache coheret protocol: \t"<< sce.read0_ch<<endl;
    
    cout << "\t cpu1 read: \t"<< sce.read1<<endl;
    cout << "\t cpu1 read active cache coheret protocol: \t"<< sce.read1_ch<<endl;
    
    cout << "\t cpu0 write: \t"<< sce.write0<<endl;
    cout << "\t cpu0 write active cache coheret protocol: \t"<< sce.write0_ch<<endl;
    
    cout << "\t cpu1 write: \t"<< sce.write1<<endl;
    cout << "\t cpu1 write active cache coheret protocol: \t"<< sce.write1_ch<<endl;
    
    cout << "\t write back_0: \t"<< sce.wb0<<endl;
    cout << "\t write back_1: \t"<< sce.wb1<<endl;
    
    if(sce.active_t.size()!=0){
        cout<<"active flow specification states: "<<endl;
        
        active_list ac_l=sort(sce.active_t);
        
        cout<<"cpu0 read: ";
        for(uint32_t i=0;i<ac_l.rd0.size();i++){
            uint32_t cfg=ac_l.rd0.at(i);
            cout<<"<"<<state(cfg)<<">  ";
            flow_inst_cnt.at(0)++;
        }
        cout<<endl;
        
        
        cout<<"cpu1 read: ";
        for(uint32_t i=0;i<ac_l.rd1.size();i++){
            uint32_t cfg=ac_l.rd1.at(i);
            cout<<"<"<<state(cfg)<<">  ";
            flow_inst_cnt.at(1)++;
        }
        cout<<endl;
        
        
        cout<<"cpu0 write: ";
        for(uint32_t i=0;i<ac_l.wt0.size();i++){
            uint32_t cfg=ac_l.wt0.at(i);
            cout<<"<"<<state(cfg)<<">  ";
            flow_inst_cnt.at(2)++;
        }
        cout<<endl;
        
        cout<<"cpu1 write: ";
        for(uint32_t i=0;i<ac_l.wt1.size();i++){
            uint32_t cfg=ac_l.wt1.at(i);
            cout<<"<"<<state(cfg)<<">  ";
            flow_inst_cnt.at(3)++;
        }
        cout<<endl;
        
        cout<<"write back_0: ";
        for(uint32_t i=0;i<ac_l.wb0.size();i++){
            uint32_t cfg=ac_l.wb0.at(i);
            cout<<"<"<<state(cfg)<<">  ";
            flow_inst_cnt.at(4)++;
        }
        cout<<endl;
        
        cout<<"write back_1: ";
        for(uint32_t i=0;i<ac_l.wb1.size();i++){
            uint32_t cfg=ac_l.wb1.at(i);
            cout<<"<"<<state(cfg)<<">  ";
            flow_inst_cnt.at(5)++;
        }
        cout<<endl;
        
        
        cout << "total flow instances:" << endl;
        for (uint32_t i = 0; i < flow_inst_cnt.size(); i++) {
            lpn_t* flow = flow_spec.at(i);
            cout << "\t" << flow->get_flow_name() << ": \t" << flow_inst_cnt.at(flow->get_index()) << endl;
        }
    }
    cout << endl;
}

bool equalscen(const scenario_t &x, const scenario_t &y){
    if(x.read0!=y.read0||x.read1!=y.read1||x.write0!=y.write0||x.write1!=y.write1)
        return false;
    if(x.active_t.size()!=y.active_t.size())
        return false;
    return true;
}
bool equalact(const active_list &fi, const active_list &se){
    //cout<<"compare rd0"<<endl;
    if(fi.rd0.size()==se.rd0.size()){
        for(uint32_t i=0;i<fi.rd0.size();i++){
            //cout<<fi.rd0.at(i)<<", "<<se.rd0.at(i)<<endl;
            if (fi.rd0.at(i)!=se.rd0.at(i))
                return false;
        }
    }
    else
        return false;
    
    if(fi.rd1.size()==se.rd1.size()){
        for(uint32_t i=0;i<fi.rd1.size();i++){
            if (fi.rd1.at(i)!=se.rd1.at(i))
                return false;
        }
    }
    else
        return false;
    
    if(fi.wt0.size()==se.wt0.size()){
        for(uint32_t i=0;i<fi.wt0.size();i++){
            if (fi.wt0.at(i)!=se.wt0.at(i))
                return false;
        }
    }
    else
        return false;
    
    if(fi.wt1.size()==se.wt1.size()){
        for(uint32_t i=0;i<fi.wt1.size();i++){
            if (fi.wt1.at(i)!=se.wt1.at(i))
                return false;
        }
    }
    else
        return false;
    
    if(fi.wb0.size()==se.wb0.size()){
        for(uint32_t i=0;i<fi.wb0.size();i++){
            if (fi.wb0.at(i)!=se.wb0.at(i))
                return false;
        }
    }
    else
        return false;
    
    if(fi.wb1.size()==se.wb1.size()){
        for(uint32_t i=0;i<fi.wb1.size();i++){
            if (fi.wb1.at(i)!=se.wb1.at(i))
                return false;
        }
    }
    else
        return false;
    
    //cout<<"return true";
    /**
     for(uint32_t i=0;i<x.active_t.size();i++){
     if(x.active_t.at(i).flow_inst->get_flow_name()!=y.active_t.at(i).flow_inst->get_flow_name())
     return false;
     
     if(x.active_t.at(i).cfg!=y.active_t.at(i).cfg)
     return false;
     
     }
     
     **/
    return true;
}

vector<scenario_t> dscen(const vector<scenario_t> &vec){
    
    vector<scenario_t> rst;
    vector<active_list> ac_vec;
    vector<int> red;
    //rst.push_back(vec.at(0));
    
    for(uint32_t i=0;i<vec.size();i++)
        ac_vec.push_back(sort(vec.at(i).active_t));
    
    for(uint32_t i=0;i<vec.size();i++){
        if(find(red.begin(), red.end(), i) == red.end()){
            //cout<<"pushed "<<i<<endl;
            rst.push_back(vec.at(i));
            for(uint32_t j=i+1; j< vec.size(); j++){
                if(equalscen(vec.at(i),vec.at(j))&&equalact(ac_vec.at(i),ac_vec.at(j))){
                    red.push_back(j);
                }
            }
        }
    }
    return rst;
}

string cfg_str_c(const uint32_t& xcfg){
    uint32_t cfg=xcfg;
    string cfg_str;
    bool cfg_convert_begin = true;
    for (uint32_t i = 0; i < 32; i++) {
        if ((cfg & 1) == 1 ) {
            if (cfg_convert_begin) {
                cfg_str = to_string(i);
                cfg_convert_begin = false;
            }
            else
                cfg_str += " " + to_string(i);
        }
        cfg = cfg >> 1;
    }
    return cfg_str;
}


//write_back flow
lpn_t* build_wb1(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** writeback1******");
    
    message_t msg2;
    msg2.pre_cfg = (1<<0);
    msg2.post_cfg = (1 << 1);
    msg2.src = cache1;
    msg2.dest = membus;
    msg2.cmd = wb;
    lpn->insert_msg(msg2);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = membus;
    msg15.dest = mem;
    msg15.cmd = wb;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 17);
    msg16.src = mem;
    msg16.dest = membus;
    msg16.cmd = wb;
    lpn->insert_msg(msg16);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* build_wb0(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** writeback0******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cache0;
    msg1.dest = membus;
    msg1.cmd = wb;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = membus;
    msg15.dest = mem;
    msg15.cmd = wb;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 17);
    msg16.src = mem;
    msg16.dest = membus;
    msg16.cmd = wb;
    lpn->insert_msg(msg16);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* poweron_gfx(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** gfx power on******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwron;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = gfx;
    msg15.cmd = pwron;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = gfx;
    msg16.dest = pwr;
    msg16.cmd = pwron;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwron;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* poweroff_gfx(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** gfx power off******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwroff;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = gfx;
    msg15.cmd = pwroff;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = gfx;
    msg16.dest = pwr;
    msg16.cmd = pwroff;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwroff;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}


lpn_t* poweron_audio(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** audio power on******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwron;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = audio;
    msg15.cmd = pwron;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = audio;
    msg16.dest = pwr;
    msg16.cmd = pwron;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwron;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* poweroff_audio(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** audio power off******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwroff;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = audio;
    msg15.cmd = pwroff;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = audio;
    msg16.dest = pwr;
    msg16.cmd = pwroff;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwroff;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}


lpn_t* poweron_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** usb power on******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwron;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = usb;
    msg15.cmd = pwron;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = usb;
    msg16.dest = pwr;
    msg16.cmd = pwron;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwron;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* poweroff_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** usb power off******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwroff;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = usb;
    msg15.cmd = pwroff;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = usb;
    msg16.dest = pwr;
    msg16.cmd = pwroff;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwroff;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}


lpn_t* poweron_uart(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** uart power on******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwron;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = uart;
    msg15.cmd = pwron;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = uart;
    msg16.dest = pwr;
    msg16.cmd = pwron;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwron;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* poweroff_uart(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("**** uart power off******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = membus;
    msg1.dest = pwr;
    msg1.cmd = pwroff;
    lpn->insert_msg(msg1);
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 2);
    msg15.src = pwr;
    msg15.dest = uart;
    msg15.cmd = pwroff;
    lpn->insert_msg(msg15);
    
    message_t msg16;
    msg16.pre_cfg = (1<<2);
    msg16.post_cfg = (1 << 3);
    msg16.src = uart;
    msg16.dest = pwr;
    msg16.cmd = pwroff;
    lpn->insert_msg(msg16);
    
    message_t msg6;
    msg6.pre_cfg = (1<<3);
    msg6.post_cfg = (1 << 17);
    msg6.src = pwr;
    msg6.dest = membus;
    msg6.cmd = pwroff;
    lpn->insert_msg(msg6);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}



lpn_t* build_cpu1_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = mem;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = mem;
    msg26.dest = bus1;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_write(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1_write*******");
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = mem;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = mem;
    msg26.dest = bus1;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = mem;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = mem;
    msg26.dest = bus0;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_write(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0_write*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = mem;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = mem;
    msg26.dest = bus0;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_read_audio(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = audio;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = audio;
    msg26.dest = bus1;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_write_audio(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1_write*******");
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = audio;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = audio;
    msg26.dest = bus1;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_read_audio(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = audio;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = audio;
    msg26.dest = bus0;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_write_audio(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0_write*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = audio;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = audio;
    msg26.dest = bus0;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_read_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = usb;
    msg26.dest = bus1;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_write_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1_write*******");
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = usb;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = usb;
    msg26.dest = bus1;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_read_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = usb;
    msg26.dest = bus0;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_write_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0_write*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = usb;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = usb;
    msg26.dest = bus0;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_read_gfx(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = gfx;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = gfx;
    msg26.dest = bus1;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_write_gfx(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1_write*******");
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = gfx;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = gfx;
    msg26.dest = bus1;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_read_gfx(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = gfx;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = gfx;
    msg26.dest = bus0;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_write_gfx(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0_write*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = gfx;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = gfx;
    msg26.dest = bus0;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_read_uart(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = uart;
    msg26.dest = bus1;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu1_write_uart(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1_write*******");
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu1;
    msg1.dest = cache1;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache1;
    msg15.dest = cpu1;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache1;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache1;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus1;
    msg25.dest = uart;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = uart;
    msg26.dest = bus1;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache1;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache1;
    msg28.dest = cpu1;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_read_uart(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 read*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = rd;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = rd;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = uart;
    msg26.dest = bus0;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = rd;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_cpu0_write_uart(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0_write*******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = cpu0;
    msg1.dest = cache0;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg15;
    msg15.pre_cfg = (1<<1);
    msg15.post_cfg = (1 << 17);
    msg15.src = cache0;
    msg15.dest = cpu0;
    msg15.cmd = wt;
    lpn->insert_msg(msg15);
    
    
    message_t msg22;
    msg22.pre_cfg = (1<<1);
    msg22.post_cfg = (1 << 2);
    msg22.src = cache0;
    msg22.dest = membus;
    msg22.cmd = wt;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache1;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = membus;
    msg29.dest = cache0;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<8);
    msg30.post_cfg =(1<<16);
    msg30.src = cache0;
    msg30.dest = cpu0;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = bus0;
    msg25.dest = uart;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<5);
    msg26.post_cfg = (1<<6);
    msg26.src = uart;
    msg26.dest = bus0;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    message_t msg27;
    msg27.pre_cfg = (1<<6);
    msg27.post_cfg = (1 <<7);
    msg27.src = membus;
    msg27.dest = cache0;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    message_t msg28;
    msg28.pre_cfg = (1<<7);
    msg28.post_cfg = (1<<17);
    msg28.src = cache0;
    msg28.dest = cpu0;
    msg28.cmd = wt;
    lpn->insert_msg(msg28);
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}


lpn_t* build_gfx_upstream_write(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****gfx upstream write *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = gfx;
    msg1.dest = membus;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<4);
    msg2.post_cfg = (1<<5);
    msg2.src = membus;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<5);
    msg3.post_cfg = (1<<6);
    msg3.src = cache1;
    msg3.dest = membus;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<16);
    msg29.src = membus;
    msg29.dest = gfx;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = gfx;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<6);
    msg25.post_cfg = (1<<7);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<7);
    msg26.post_cfg = (1<<10);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    
    message_t msg5;
    msg5.pre_cfg = (1<<6);
    msg5.post_cfg = (1<<8);
    msg5.src = membus;
    msg5.dest = usb;
    msg5.cmd = wt;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = usb;
    msg6.dest = membus;
    msg6.cmd = wt;
    lpn->insert_msg(msg6);
    
    message_t msg7;
    msg7.pre_cfg = (1<<6);
    msg7.post_cfg = (1<<9);
    msg7.src = membus;
    msg7.dest = audio;
    msg7.cmd = wt;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = audio;
    msg8.dest = membus;
    msg8.cmd = wt;
    lpn->insert_msg(msg8);

    
    message_t msg27;
    msg27.pre_cfg = (1<<10);
    msg27.post_cfg = (1 <<17);
    msg27.src = membus;
    msg27.dest = gfx;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);

    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* build_gfx_upstream_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****gfx upstream write *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = gfx;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<4);
    msg2.post_cfg = (1<<5);
    msg2.src = membus;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<5);
    msg3.post_cfg = (1<<6);
    msg3.src = cache1;
    msg3.dest = membus;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<16);
    msg29.src = membus;
    msg29.dest = gfx;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = gfx;
    msg30.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<6);
    msg25.post_cfg = (1<<7);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<7);
    msg26.post_cfg = (1<<10);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    
    message_t msg5;
    msg5.pre_cfg = (1<<6);
    msg5.post_cfg = (1<<8);
    msg5.src = membus;
    msg5.dest = usb;
    msg5.cmd = rd;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = usb;
    msg6.dest = membus;
    msg6.cmd = rd;
    lpn->insert_msg(msg6);
    
    message_t msg7;
    msg7.pre_cfg = (1<<6);
    msg7.post_cfg = (1<<9);
    msg7.src = membus;
    msg7.dest = audio;
    msg7.cmd = rd;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = audio;
    msg8.dest = membus;
    msg8.cmd = rd;
    lpn->insert_msg(msg8);
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<10);
    msg27.post_cfg = (1 <<17);
    msg27.src = membus;
    msg27.dest = gfx;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* build_audio_upstream_write(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****audio upstream write *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = audio;
    msg1.dest = membus;
    msg1.cmd = wt;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<4);
    msg2.post_cfg = (1<<5);
    msg2.src = membus;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<5);
    msg3.post_cfg = (1<<6);
    msg3.src = cache1;
    msg3.dest = membus;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<16);
    msg29.src = membus;
    msg29.dest = gfx;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = gfx;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<6);
    msg25.post_cfg = (1<<7);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = wt;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<7);
    msg26.post_cfg = (1<<10);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = wt;
    lpn->insert_msg(msg26);
    
    
    message_t msg5;
    msg5.pre_cfg = (1<<6);
    msg5.post_cfg = (1<<8);
    msg5.src = membus;
    msg5.dest = usb;
    msg5.cmd = wt;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = usb;
    msg6.dest = membus;
    msg6.cmd = wt;
    lpn->insert_msg(msg6);
    
    message_t msg7;
    msg7.pre_cfg = (1<<6);
    msg7.post_cfg = (1<<9);
    msg7.src = membus;
    msg7.dest = audio;
    msg7.cmd = wt;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = audio;
    msg8.dest = membus;
    msg8.cmd = wt;
    lpn->insert_msg(msg8);
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<10);
    msg27.post_cfg = (1 <<17);
    msg27.src = membus;
    msg27.dest = audio;
    msg27.cmd = wt;
    lpn->insert_msg(msg27);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
lpn_t* build_audio_upstream_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****audio upstream write *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = audio;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<4);
    msg2.post_cfg = (1<<5);
    msg2.src = membus;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<5);
    msg3.post_cfg = (1<<6);
    msg3.src = cache1;
    msg3.dest = membus;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<16);
    msg29.src = membus;
    msg29.dest = audio;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = audio;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<6);
    msg25.post_cfg = (1<<7);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<7);
    msg26.post_cfg = (1<<10);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    
    message_t msg5;
    msg5.pre_cfg = (1<<6);
    msg5.post_cfg = (1<<8);
    msg5.src = membus;
    msg5.dest = usb;
    msg5.cmd = rd;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = usb;
    msg6.dest = membus;
    msg6.cmd = rd;
    lpn->insert_msg(msg6);
    
    message_t msg7;
    msg7.pre_cfg = (1<<6);
    msg7.post_cfg = (1<<9);
    msg7.src = membus;
    msg7.dest = gfx;
    msg7.cmd = rd;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = gfx;
    msg8.dest = membus;
    msg8.cmd = rd;
    lpn->insert_msg(msg8);
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<10);
    msg27.post_cfg = (1 <<17);
    msg27.src = membus;
    msg27.dest = audio;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}

lpn_t* build_usb_upstream_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****usb upstream write *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = usb;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<4);
    msg2.post_cfg = (1<<5);
    msg2.src = membus;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<5);
    msg3.post_cfg = (1<<6);
    msg3.src = cache1;
    msg3.dest = membus;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<16);
    msg29.src = membus;
    msg29.dest = usb;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = usb;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<6);
    msg25.post_cfg = (1<<7);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<7);
    msg26.post_cfg = (1<<10);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    
    message_t msg5;
    msg5.pre_cfg = (1<<6);
    msg5.post_cfg = (1<<8);
    msg5.src = membus;
    msg5.dest = audio;
    msg5.cmd = rd;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = audio;
    msg6.dest = membus;
    msg6.cmd = rd;
    lpn->insert_msg(msg6);
    
    message_t msg7;
    msg7.pre_cfg = (1<<6);
    msg7.post_cfg = (1<<9);
    msg7.src = membus;
    msg7.dest = gfx;
    msg7.cmd = rd;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = gfx;
    msg8.dest = membus;
    msg8.cmd = rd;
    lpn->insert_msg(msg8);
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<10);
    msg27.post_cfg = (1 <<17);
    msg27.src = membus;
    msg27.dest = usb;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;



}


lpn_t* build_uart_upstream_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****uart upstream write *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = uart;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<3);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<4);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<4);
    msg2.post_cfg = (1<<5);
    msg2.src = membus;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<5);
    msg3.post_cfg = (1<<6);
    msg3.src = cache1;
    msg3.dest = membus;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<16);
    msg29.src = membus;
    msg29.dest = uart;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = uart;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<6);
    msg25.post_cfg = (1<<7);
    msg25.src = membus;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<7);
    msg26.post_cfg = (1<<10);
    msg26.src = usb;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    
    message_t msg5;
    msg5.pre_cfg = (1<<6);
    msg5.post_cfg = (1<<8);
    msg5.src = membus;
    msg5.dest = audio;
    msg5.cmd = rd;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = audio;
    msg6.dest = membus;
    msg6.cmd = rd;
    lpn->insert_msg(msg6);
    
    message_t msg7;
    msg7.pre_cfg = (1<<6);
    msg7.post_cfg = (1<<9);
    msg7.src = membus;
    msg7.dest = gfx;
    msg7.cmd = rd;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = gfx;
    msg8.dest = membus;
    msg8.cmd = rd;
    lpn->insert_msg(msg8);
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<10);
    msg27.post_cfg = (1 <<17);
    msg27.src = membus;
    msg27.dest = uart;
    msg27.cmd = rd;
    lpn->insert_msg(msg27);
    
    
    lpn->set_init_cfg(1<<0);
    
    return lpn;
}
