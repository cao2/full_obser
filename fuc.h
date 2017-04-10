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
uint32_t num_flow=35;
vector<string> flow_names;
void init(){
    flow_names.push_back("cpu0 read");
    flow_names.push_back("cpu1 read");
    flow_names.push_back("cpu0 write");
    flow_names.push_back("cpu1 write");
    flow_names.push_back("cpu0 write back");
    flow_names.push_back("cpu1 write back");
    flow_names.push_back("cpu0 read audio");
    flow_names.push_back("cpu0 write audio");
    flow_names.push_back("cpu1 read audio");
    
    flow_names.push_back("cpu1 write audio");
    flow_names.push_back("cpu0 read gfx");
     flow_names.push_back("cpu0 write gfx");
    flow_names.push_back("cpu1 read gfx");
   
    flow_names.push_back("cpu1 write gfx");
    flow_names.push_back("cpu0 read usb");
     flow_names.push_back("cpu0 write usb");
    flow_names.push_back("cpu1 read usb");
   
    flow_names.push_back("cpu1 write usb");
    flow_names.push_back("cpu0 read uart");
     flow_names.push_back("cpu0 write uart");
    flow_names.push_back("cpu1 read uart");
   
    flow_names.push_back("cpu1 write uart");
    flow_names.push_back("poweron uart");
    flow_names.push_back("poweroff uart");
    flow_names.push_back("poweron usb");
    flow_names.push_back("poweroff usb");
    flow_names.push_back("poweron audio");
    flow_names.push_back("poweroff audio");
    flow_names.push_back("poweron gfx");
    flow_names.push_back("poweroff gfx");
    flow_names.push_back("gfx upstream write");
    flow_names.push_back("gfx upstream read");
    flow_names.push_back("audio upstream write");
    flow_names.push_back("audio upstream read");
    flow_names.push_back("usb upstream read");
    flow_names.push_back("uart upstream read");
    flow_names.push_back("cpu0 read cache coh");
    flow_names.push_back("cpu1 read cache coh");
    flow_names.push_back("cpu0 write cache coh");
    flow_names.push_back("cpu1 write cache coh");
}
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
struct order_inst
{
    bool start;
    uint32_t flow;
    uint32_t addr;
    uint32_t time;
    order_inst(){
        start=true;
        flow=0;
        addr=0;
        time=0;
    }
};
struct flow_instance_t
{
    lpn_t* flow_inst;
    config_t cfg;
    uint32_t addr;
    uint32_t time;
    flow_instance_t() {
        flow_inst = nullptr;
        cfg = null_cfg;
        time=0;
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
    vector< vector<uint32_t> > active_inst;
    //rd0,rd1,wt0,wt1,wb0,wb1,rd0_gfx,rd0_audio,rd0_usb,rd0_uart,rd1_gfx,rd1_audio,rd1_usb,rd1_uart,wt0_gfx,wt0_audio,wt0_usb,wt0_uart,wt1_gfx,wt1_audio,wt1_usb,wt1_uart,pwron_gfx,pwron_audio,pwron_usb,pwron_uart,pwroff_gfx,pwroff_audio,pwroff_usb,pwroff_uart,gfx_up_rd,gfx_up_wt,usb_up_rd,usb_up_wt,uart_up_rd,uart_up_wt,audio_up_rd,audio_up_wt;
    active_list(){
        for (uint32_t i=0; i<num_flow+4;i++){
            vector<uint32_t> tmp;
            active_inst.push_back(tmp);
        }
    }
    void sortall(){
        for(uint32_t i=0; i<active_inst.size();i++)
            sort(active_inst.at(i).begin(),active_inst.at(i).end());
    }
};

struct scenario_t{
    vector<uint32_t> finished;
    
    vector<flow_instance_t> active_t;
    //active_list active_sort;
    vector<order_inst> order;
    //vector<uint32_t> order_addr;
    scenario_t(){
        for( uint32_t i=0;i<num_flow+4;i++)
            finished.push_back(0);
      
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
        //cout<<"active: ind:"<<inde<<endl;
        inde=active_t.at(i).flow_inst->get_index();
        sorted.active_inst.at(inde).push_back(active_t.at(i).cfg);
        
    }
    sorted.sortall();
    return sorted;
    
}

void print_scenario(const vector<lpn_t*> flow_spec, const scenario_t& sce)
{
    vector<flow_instance_t> scen=sce.active_t;
    //cout<<"sce.finishe: size:"<<sce.finished.size()<<endl;
    //for(uint32_t mi:sce.finished)
    //    cout<<mi<<endl;
    //cout<<"......"<<endl;
    vector<uint32_t> flow_inst_cnt;
    for(uint32_t i=0;i<num_flow;i++){
        if (i<4)
            flow_inst_cnt.push_back(sce.finished.at(i)+sce.finished.at(i+num_flow));
        else
            flow_inst_cnt.push_back(sce.finished.at(i));
        
    }
    cout<<"==============================================="<<endl;
    cout << "finished flow instances:" << endl;
    
    for(uint32_t j=0;j<sce.finished.size();j++){
        if (sce.finished.at(j)){
            if (j< num_flow)
                if (sce.finished.at(j)>0){
                    cout<<"        *****"<<flow_names.at(j)<<": ";
                    cout<<sce.finished.at(j)<<endl;}
        }
    }
    cout<<"==============================================="<<endl;
    if(sce.active_t.size()!=0){
        cout<<"active flow specification states: "<<endl;
        
        active_list ac_l=sort(sce.active_t);
        
        for(uint32_t j=0;j<ac_l.active_inst.size();j++){
            if (ac_l.active_inst.at(j).size()>0){
                cout<<"        *****"<<flow_names.at(j)<<": ";
                for(uint32_t i=0;i<ac_l.active_inst.at(j).size();i++){
                    uint32_t cfg=ac_l.active_inst.at(j).at(i);
                    cout<<"<"<<state(cfg)<<">  ";
                    flow_inst_cnt.at(j)++;
                }
                cout<<endl;
            }
        }
       cout<<"==============================================="<<endl;
        cout << "total flow instances:" << endl;
        for (uint32_t i = 0; i < flow_inst_cnt.size(); i++) {
            lpn_t* flow = flow_spec.at(i);
            if (flow_inst_cnt.at(flow->get_index())>0)
            cout << "\t" << flow->get_flow_name() << ": \t" << flow_inst_cnt.at(flow->get_index()) << endl;
        }
        cout<<"==============================================="<<endl;
        cout<<"==============================================="<<endl;
    }
    cout << endl;
}
//add more
bool equalscen(const scenario_t &x, const scenario_t &y){
    for (uint32_t i=0;i<x.finished.size();i++){
        if (x.finished.at(i)!=y.finished.at(i))
            return false;
    }
    
    if(x.active_t.size()!=y.active_t.size())
        return false;
    return true;
}
bool equalact(const active_list &fi, const active_list &se){
    
    for (uint32_t i=0;i<fi.active_inst.size();i++){
        if (fi.active_inst.at(i).size()==se.active_inst.at(i).size()){
            for (uint32_t j=0;j<fi.active_inst.at(i).size();j++)
                if (fi.active_inst.at(i).at(j)!=se.active_inst.at(i).at(j)) {
                    return false;
                }
        }
        else
            return false;
        
    }
    
   
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
lpn_t* build_poweron_gfx(void){
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
lpn_t* build_poweroff_gfx(void){
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
lpn_t* build_poweron_audio(void){
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

lpn_t* build_poweroff_audio(void){
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
lpn_t* build_poweron_usb(void){
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
lpn_t* build_poweroff_usb(void){
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
lpn_t* build_poweron_uart(void){
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
lpn_t* build_poweroff_uart(void){
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
  
    
    message_t msg29;
    msg29.pre_cfg = (1<<3);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = mem;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = mem;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = mem;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = mem;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = mem;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = mem;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = mem;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = mem;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = mem;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = mem;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = mem;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = mem;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = mem;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = mem;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = mem;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = mem;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = mem;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = mem;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = mem;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = mem;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = mem;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = mem;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = mem;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = mem;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = mem;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = mem;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = mem;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = mem;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = mem;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = mem;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = mem;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = mem;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = mem;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = mem;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
  
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = mem;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = mem;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = mem;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = mem;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = mem;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = mem;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = mem;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = mem;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = mem;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = mem;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = mem;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = mem;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = mem;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = mem;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = mem;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = mem;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = mem;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu0 write*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = mem;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = mem;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = mem;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = mem;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = mem;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = mem;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = mem;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = mem;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = mem;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = mem;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = mem;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = mem;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = mem;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = mem;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = mem;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = mem;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = mem;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1 read gfx*******");
    
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<3);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = gfx;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = gfx;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg33;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg33.src = gfx;
    msg33.dest = membus;
    msg33.cmd = rd;
    lpn->insert_msg(msg33);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = gfx;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = gfx;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = gfx;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = gfx;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = gfx;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = gfx;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = gfx;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = gfx;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = gfx;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = gfx;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = gfx;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = gfx;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = gfx;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = gfx;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1_write gfx*******");
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = gfx;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = gfx;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = gfx;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = gfx;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = gfx;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = gfx;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = gfx;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = gfx;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = gfx;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = gfx;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = gfx;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = gfx;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = gfx;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = gfx;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = gfx;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = gfx;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = gfx;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
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
    
    lpn->set_flow_name("****cpu0 read gfx*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = gfx;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = gfx;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = gfx;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = gfx;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = gfx;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = gfx;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = gfx;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = gfx;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = gfx;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = gfx;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = gfx;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = gfx;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = gfx;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = gfx;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = gfx;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = gfx;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = gfx;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu0 write gfx*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = gfx;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = gfx;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = gfx;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = gfx;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = gfx;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = gfx;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = gfx;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = gfx;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = gfx;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = gfx;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = gfx;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = gfx;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = gfx;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = gfx;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = gfx;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = gfx;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = gfx;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1 read usb*******");
    
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<3);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = usb;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = usb;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = usb;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = usb;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = usb;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = usb;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = usb;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = usb;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = usb;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = usb;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = usb;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = usb;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = usb;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = usb;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = usb;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = usb;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
lpn_t* build_cpu0_read_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 read usb*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = usb;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = usb;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = usb;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = usb;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = usb;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = usb;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = usb;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = usb;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = usb;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = usb;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = usb;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = usb;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = usb;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = usb;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = usb;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = usb;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
lpn_t* build_cpu1_write_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu1 write usb*******");
    
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<3);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = usb;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = usb;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = usb;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = usb;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = usb;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = usb;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = usb;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = usb;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = usb;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = usb;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = usb;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = usb;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = usb;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = usb;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = usb;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = usb;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
lpn_t* build_cpu0_write_usb(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****cpu0 write usb*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = usb;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = usb;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = usb;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = usb;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = usb;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = usb;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = usb;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = usb;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = usb;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = usb;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = usb;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = usb;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = usb;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = usb;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = usb;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = usb;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = usb;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1 read*uart******");
    
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<3);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = uart;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = uart;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = uart;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = uart;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = uart;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = uart;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = uart;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = uart;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = uart;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = uart;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = uart;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = uart;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = uart;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = uart;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = uart;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1_write uart*******");
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = uart;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = uart;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = uart;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = uart;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = uart;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = uart;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = uart;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = uart;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = uart;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = uart;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = uart;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = uart;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = uart;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = uart;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = uart;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu0 read uart*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = uart;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = uart;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = uart;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = uart;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = uart;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = uart;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = uart;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = uart;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = uart;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = uart;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = uart;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = uart;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = uart;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = uart;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = uart;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu0 write uart*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = uart;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = uart;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = uart;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = uart;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = uart;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = uart;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = uart;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = uart;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = uart;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = uart;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = uart;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = uart;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = uart;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = uart;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = uart;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = uart;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = uart;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1 read audio*******");
    
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<3);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = rd;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = audio;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = audio;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = audio;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = audio;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = audio;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = audio;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = audio;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = audio;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = audio;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = audio;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = audio;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = audio;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = audio;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = audio;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = audio;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = audio;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = audio;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu1_write audio*******");
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
    msg22.dest = cache0;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache0;
    msg23.dest = cache1;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg29;
    msg29.pre_cfg = (1<<4);
    msg29.post_cfg = (1<<8);
    msg29.src = cache1;
    msg29.dest = membus;
    msg29.cmd = wt;
    lpn->insert_msg(msg29);
    
    message_t msg30;
    msg30.pre_cfg = (1<<4);
    msg30.post_cfg =(1<<16);
    msg30.src = cache1;
    msg30.dest = cpu1;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<8);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = audio;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = audio;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = audio;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = audio;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = audio;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = audio;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = audio;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = audio;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = audio;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = audio;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = audio;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = audio;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = audio;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = audio;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = audio;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = audio;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = audio;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu0 read audio*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = audio;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = audio;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = audio;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = audio;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = audio;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = audio;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = audio;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = audio;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = audio;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = audio;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = audio;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = audio;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = audio;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = audio;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = audio;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = audio;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = audio;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    
    lpn->set_flow_name("****cpu0 write audio*******");
    
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
    msg22.dest = cache1;
    msg22.cmd = snp;
    lpn->insert_msg(msg22);
    
    message_t msg23;
    msg23.pre_cfg = (1<<2);
    msg23.post_cfg = (1<<3);
    msg23.src = cache1;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<3);
    msg24.post_cfg = (1<<16);
    msg24.src = cache0;
    msg24.dest = cpu0;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<3);
    msg30.post_cfg =(1<<4);
    msg30.src = cache0;
    msg33.dest = membus;
    msg30.cmd = wt;
    lpn->insert_msg(msg30);
    
    message_t msg25;
    msg25.pre_cfg = (1<<4);
    msg25.post_cfg = (1<<5);
    msg25.src = membus;
    msg25.dest = audio;
    msg25.cmd = rd;
    lpn->insert_msg(msg25);
    
    //16 responses
    message_t msg26;
    msg26.pre_cfg = (1<<6);
    msg26.post_cfg = (1<<7);
    msg26.src = audio;
    msg26.dest = membus;
    msg26.cmd = rd;
    lpn->insert_msg(msg26);
    
    message_t msg30;
    msg33.pre_cfg = (1<<7);
    msg33.post_cfg = (1<<8);
    msg30.src = audio;
    msg33.dest = membus;
    msg30.cmd = rd;
    lpn->insert_msg(msg30);
    
    message_t msg31;
    msg31.pre_cfg = (1<<8);
    msg31.post_cfg = (1<<9);
    msg31.src = audio;
    msg31.dest = membus;
    msg31.cmd = rd;
    lpn->insert_msg(msg31);
    
    message_t msg40;
    msg40.pre_cfg = (1<<9);
    msg40.post_cfg = (1<<10);
    msg40.src = audio;
    msg40.dest = membus;
    msg40.cmd = rd;
    lpn->insert_msg(msg40);
    
    message_t msg41;
    msg41.pre_cfg = (1<<11);
    msg41.post_cfg = (1<<12);
    msg41.src = audio;
    msg41.dest = membus;
    msg41.cmd = rd;
    lpn->insert_msg(msg41);
    
    
    message_t msg50;
    msg50.pre_cfg = (1<<12);
    msg50.post_cfg = (1<<13);
    msg50.src = audio;
    msg50.dest = membus;
    msg50.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg52;
    msg52.pre_cfg = (1<<13);
    msg52.post_cfg = (1<<14);
    msg52.src = audio;
    msg52.dest = membus;
    msg52.cmd = rd;
    lpn->insert_msg(msg50);
    
    message_t msg51;
    msg51.pre_cfg = (1<<14);
    msg51.post_cfg = (1<<15);
    msg51.src = audio;
    msg51.dest = membus;
    msg51.cmd = rd;
    lpn->insert_msg(msg51);
    
    message_t msg53;
    msg53.pre_cfg = (1<<15);
    msg53.post_cfg = (1<<16);
    msg53.src = audio;
    msg53.dest = membus;
    msg53.cmd = rd;
    lpn->insert_msg(msg53);
    
    message_t msg54;
    msg54.pre_cfg = (1<<16);
    msg54.post_cfg = (1<<20);
    msg54.src = audio;
    msg54.dest = membus;
    msg54.cmd = rd;
    lpn->insert_msg(msg54);
    
    message_t msg60;
    msg60.pre_cfg = (1<<20);
    msg60.post_cfg = (1<<21);
    msg60.src = audio;
    msg60.dest = membus;
    msg60.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg62;
    msg62.pre_cfg = (1<<21);
    msg62.post_cfg = (1<<22);
    msg62.src = audio;
    msg62.dest = membus;
    msg62.cmd = rd;
    lpn->insert_msg(msg60);
    
    message_t msg61;
    msg61.pre_cfg = (1<<22);
    msg61.post_cfg = (1<<23);
    msg61.src = audio;
    msg61.dest = membus;
    msg61.cmd = rd;
    lpn->insert_msg(msg61);
    
    message_t msg63;
    msg63.pre_cfg = (1<<23);
    msg63.post_cfg = (1<<24);
    msg63.src = audio;
    msg63.dest = membus;
    msg63.cmd = rd;
    lpn->insert_msg(msg63);
    
    message_t msg64;
    msg64.pre_cfg = (1<<24);
    msg64.post_cfg = (1<<25);
    msg64.src = audio;
    msg64.dest = membus;
    msg64.cmd = rd;
    lpn->insert_msg(msg64);
    
    message_t msg70;
    msg70.pre_cfg = (1<<25);
    msg70.post_cfg = (1<<26);
    msg70.src = audio;
    msg70.dest = membus;
    msg70.cmd = rd;
    lpn->insert_msg(msg70);
    
    message_t msg27;
    msg27.pre_cfg = (1<<26);
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
    msg23.post_cfg = (1<<2);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<2);
    msg24.post_cfg = (1<<6);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<2);
    msg2.post_cfg = (1<<4);
    msg2.src = cache0;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<4);
    msg3.post_cfg = (1<<5);
    msg3.src = cache1;
    msg3.dest = cache0;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg4;
    msg4.pre_cfg=(1<<5);
    msg4.post_cfg=(1<<6);
    msg4.src=cache0;
    msg4.dest=membus;
    msg4.cmd=snp;
    lpn->insert_msg(msg4);
    
    
    
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
    
    
    
    message_t msg9;
    msg9.pre_cfg=(1<<6);
    msg9.post_cfg=(1<<11);
    msg9.src=membus;
    msg9.dest=mem;
    msg9.cmd=wt;
    lpn->insert_msg(msg9);
    
    message_t msg10;
    msg10.pre_cfg=(1<<11);
    msg10.post_cfg=(1<<10);
    msg10.src=mem;
    msg10.dest=membus;
    msg10.cmd=wt;
    lpn->insert_msg(msg10);
    
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
    msg23.post_cfg = (1<<2);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<2);
    msg24.post_cfg = (1<<6);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<2);
    msg2.post_cfg = (1<<4);
    msg2.src = cache0;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<4);
    msg3.post_cfg = (1<<5);
    msg3.src = cache1;
    msg3.dest = cache0;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg4;
    msg4.pre_cfg=(1<<5);
    msg4.post_cfg=(1<<6);
    msg4.src=cache0;
    msg4.dest=membus;
    msg4.cmd=snp;
    lpn->insert_msg(msg4);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = audio;
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
    msg7.dest = gfx;
    msg7.cmd = wt;
    lpn->insert_msg(msg7);
    
    message_t msg8;
    msg8.pre_cfg = (1<<9);
    msg8.post_cfg = (1<<10);
    msg8.src = gfx;
    msg8.dest = membus;
    msg8.cmd = wt;
    lpn->insert_msg(msg8);
    
    message_t msg9;
    msg9.pre_cfg=(1<<6);
    msg9.post_cfg=(1<<11);
    msg9.src=membus;
    msg9.dest=mem;
    msg9.cmd=wt;
    lpn->insert_msg(msg9);
    
    message_t msg10;
    msg10.pre_cfg=(1<<11);
    msg10.post_cfg=(1<<10);
    msg10.src=mem;
    msg10.dest=membus;
    msg10.cmd=wt;
    lpn->insert_msg(msg10);
    
    
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

lpn_t* build_gfx_upstream_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****gfx upstream read *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = gfx;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<2);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<2);
    msg24.post_cfg = (1<<6);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<2);
    msg2.post_cfg = (1<<4);
    msg2.src = cache0;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<4);
    msg3.post_cfg = (1<<5);
    msg3.src = cache1;
    msg3.dest = cache0;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg4;
    msg4.pre_cfg=(1<<5);
    msg4.post_cfg=(1<<6);
    msg4.src=cache0;
    msg4.dest=membus;
    msg4.cmd=snp;
    lpn->insert_msg(msg4);
    
    
    
    message_t msg30;
    msg30.pre_cfg = (1<<6);
    msg30.post_cfg =(1<<16);
    msg30.src = membus;
    msg30.dest = gfx;
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
    
    message_t msg9;
    msg9.pre_cfg=(1<<6);
    msg9.post_cfg=(1<<11);
    msg9.src=membus;
    msg9.dest=mem;
    msg9.cmd=rd;
    lpn->insert_msg(msg9);
    
    message_t msg10;
    msg10.pre_cfg=(1<<11);
    msg10.post_cfg=(1<<10);
    msg10.src=mem;
    msg10.dest=membus;
    msg10.cmd=rd;
    lpn->insert_msg(msg10);
    
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
lpn_t* build_audio_upstream_read(void){
    lpn_t* lpn = new lpn_t;
    
    lpn->set_flow_name("****audio upstream read *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = audio;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<2);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<2);
    msg24.post_cfg = (1<<6);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<2);
    msg2.post_cfg = (1<<4);
    msg2.src = cache0;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<4);
    msg3.post_cfg = (1<<5);
    msg3.src = cache1;
    msg3.dest = cache0;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg4;
    msg4.pre_cfg=(1<<5);
    msg4.post_cfg=(1<<6);
    msg4.src=cache0;
    msg4.dest=membus;
    msg4.cmd=snp;
    lpn->insert_msg(msg4);
    
    
    
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
    
    message_t msg9;
    msg9.pre_cfg=(1<<6);
    msg9.post_cfg=(1<<11);
    msg9.src=membus;
    msg9.dest=mem;
    msg9.cmd=rd;
    lpn->insert_msg(msg9);
    
    message_t msg10;
    msg10.pre_cfg=(1<<11);
    msg10.post_cfg=(1<<10);
    msg10.src=mem;
    msg10.dest=membus;
    msg10.cmd=rd;
    lpn->insert_msg(msg10);
    
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
    
    lpn->set_flow_name("****usb upstream read *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = usb;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<2);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<2);
    msg24.post_cfg = (1<<6);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<2);
    msg2.post_cfg = (1<<4);
    msg2.src = cache0;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<4);
    msg3.post_cfg = (1<<5);
    msg3.src = cache1;
    msg3.dest = cache0;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg4;
    msg4.pre_cfg=(1<<5);
    msg4.post_cfg=(1<<6);
    msg4.src=cache0;
    msg4.dest=membus;
    msg4.cmd=snp;
    lpn->insert_msg(msg4);
    
    
    
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
    msg5.dest = gfx;
    msg5.cmd = rd;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = gfx;
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
    
    message_t msg9;
    msg9.pre_cfg=(1<<6);
    msg9.post_cfg=(1<<11);
    msg9.src=membus;
    msg9.dest=mem;
    msg9.cmd=rd;
    lpn->insert_msg(msg9);
    
    message_t msg10;
    msg10.pre_cfg=(1<<11);
    msg10.post_cfg=(1<<10);
    msg10.src=mem;
    msg10.dest=membus;
    msg10.cmd=rd;
    lpn->insert_msg(msg10);
    
    
    
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
    
    lpn->set_flow_name("****uart upstream read *******");
    
    message_t msg1;
    msg1.pre_cfg = (1<<0);
    msg1.post_cfg = (1 << 1);
    msg1.src = uart;
    msg1.dest = membus;
    msg1.cmd = rd;
    lpn->insert_msg(msg1);
    
    
    message_t msg23;
    msg23.pre_cfg = (1<<1);
    msg23.post_cfg = (1<<2);
    msg23.src = membus;
    msg23.dest = cache0;
    msg23.cmd = snp;
    lpn->insert_msg(msg23);
    
    
    message_t msg24;
    msg24.pre_cfg = (1<<2);
    msg24.post_cfg = (1<<6);
    msg24.src = cache0;
    msg24.dest = membus;
    msg24.cmd = snp;
    lpn->insert_msg(msg24);
    
    message_t msg2;
    msg2.pre_cfg = (1<<2);
    msg2.post_cfg = (1<<4);
    msg2.src = cache0;
    msg2.dest = cache1;
    msg2.cmd = snp;
    lpn->insert_msg(msg2);
    
    
    message_t msg3;
    msg3.pre_cfg = (1<<4);
    msg3.post_cfg = (1<<5);
    msg3.src = cache1;
    msg3.dest = cache0;
    msg3.cmd = snp;
    lpn->insert_msg(msg3);
    
    message_t msg4;
    msg4.pre_cfg=(1<<5);
    msg4.post_cfg=(1<<6);
    msg4.src=cache0;
    msg4.dest=membus;
    msg4.cmd=snp;
    lpn->insert_msg(msg4);
    
    
    
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
    msg5.dest = gfx;
    msg5.cmd = rd;
    lpn->insert_msg(msg5);
    
    message_t msg6;
    msg6.pre_cfg = (1<<8);
    msg6.post_cfg = (1<<10);
    msg6.src = gfx;
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
    
    message_t msg9;
    msg9.pre_cfg=(1<<6);
    msg9.post_cfg=(1<<11);
    msg9.src=membus;
    msg9.dest=mem;
    msg9.cmd=rd;
    lpn->insert_msg(msg9);
    
    message_t msg10;
    msg10.pre_cfg=(1<<11);
    msg10.post_cfg=(1<<10);
    msg10.src=mem;
    msg10.dest=membus;
    msg10.cmd=rd;
    lpn->insert_msg(msg10);
    
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

