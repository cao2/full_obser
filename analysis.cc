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
#include "msgs.h"
#include "vcd_msg.h"
#include <sys/time.h>
#include <sys/resource.h>






lpn_t* build_wb0(void);
lpn_t* build_wb1(void);
lpn_t* build_cpu1_read(void);
lpn_t* build_cpu1_write(void);
lpn_t* build_cpu0_read(void);
lpn_t* build_cpu0_write(void);


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

int main(int argc, char *argv[]) {
    uint32_t max=0;
    struct rusage usage;
    struct timeval start, end;
    getrusage(RUSAGE_SELF, &usage);
    start = usage.ru_stime;
    // Build flow specification
    vector<lpn_t*> flow_spec;
    vector<uint32_t> noneed_monitors;
    
    lpn_t* cpu0_read = build_cpu0_read();
    lpn_t* cpu1_read=build_cpu1_read();
    lpn_t* cpu0_write=build_cpu0_write();
    lpn_t* cpu1_write=build_cpu1_write();
    lpn_t* write_back0=build_wb0();
    lpn_t* write_back1=build_wb1();
    
    
    flow_spec.push_back(cpu0_read);
    cpu0_read->set_index(0);
    flow_spec.push_back(cpu1_read);//write1
    cpu1_read->set_index(1);
    flow_spec.push_back(cpu0_write);
    cpu0_write->set_index(2);
    flow_spec.push_back(cpu1_write);
    cpu1_write->set_index(3);
    flow_spec.push_back(write_back0);
    write_back0->set_index(4);
    flow_spec.push_back(write_back1);
    write_back1->set_index(5);
    
    vector<uint32_t> flow_inst_cnt;
    flow_inst_cnt.push_back(0);
    flow_inst_cnt.push_back(0);
    flow_inst_cnt.push_back(0);
    flow_inst_cnt.push_back(0);
    flow_inst_cnt.push_back(0);
    flow_inst_cnt.push_back(0);
    
    ofstream errorfile;
    errorfile.open ("erromsg.txt",ios::trunc);
    
    vector<message_t> trace;
    cout<<"lala"<<endl;
    string filename(argv[1]);
    if (filename.substr(filename.size()-3).compare("vcd")==0){
        vcd_msg* readmsg= new vcd_msg;
        readmsg->parse(argv[1]);
        trace = readmsg->getMsgs();
    
    }
    else{
        msgs* readmsg= new msgs;
        readmsg->parse(argv[1]);
        trace = readmsg->getMsgs();
    }
    
    
    
    cout<<"read "<<trace.size()<<endl;
    vector<scenario_t> s_stack;
    //hash_set<scenario_t> s_stack;
    stack<uint32_t> tri_stack;
    
    s_stack.push_back(scenario_t());
    tri_stack.push(0);
    
    
    vector<pair< vector<scenario_t>,uint32_t> >  bad_scenario_vec;
    
    // Matching message in the trace to scenairos.
    bool match = false;
    while (tri_stack.size() != 0) {
        
        uint32_t tri = tri_stack.top();
        tri_stack.pop();
        
        // If index tri reaches the end of trace, store the current scenario.
        if (tri == trace.size()) {
            //break if a scenario is found to match all messages.
            break;
        }
       
        //if((tri%5==0 &&s_stack.size()>10)|| s_stack.size()>tri*tri*2){
        if((tri%5==0 )|| s_stack.size()>tri*tri*2){
            cout<<"************************"<<endl;
            cout<<"dscen called, orig size "<<s_stack.size()<<endl;
            s_stack=dscen(s_stack);
            cout<<"************************"<<endl;
            cout<<"NEW size: "<<s_stack.size()<<endl;
            
        }
        
        message_t msg(trace.at(tri));
        
        
        stack<message_t> msgs;
        msgs.push(msg);
        /**
        if (msg.cmd==rd||msg.cmd==wt){
            message_t msg2;
            if (msg.cmd==rd)
                msg2.cmd=wt;
            else
                msg2.cmd=rd;
            msg2.src=msg.src;
            msg2.dest=msg.dest;
            msg2.addr=msg.addr;
            msgs.push(msg2);
        }
         **/
        match=false;
        bool insideflag=false;
        vector<scenario_t> new_s_stack;
        while (msgs.size()!=0)
        {
            vector<config_t> flow_spec_flag;
            
            insideflag=false;
            msg=msgs.top();
            cout << tri<<"***  " << msg.toString() <<"  "<<s_stack.size() <<endl << endl;
            msgs.pop();
            
            //find out if new msg can create a new flow_inst
            for (uint32_t i = 0; i < flow_spec.size(); i++) {
                lpn_t* f = flow_spec.at(i);
                config_t new_cfg = f->accept(msg);
                if (new_cfg != null_cfg)
                    flow_spec_flag.push_back(new_cfg);
                else
                    flow_spec_flag.push_back(99);
            }
            
            // Match the next message from trace against the current scenario.
            for(uint32_t ct=0;ct<s_stack.size();ct++)
            {
                scenario_t scenario = s_stack.at(ct);
                // Match the enw_msg against the existing flow instances.
                for (uint32_t i = 0; i < scenario.active_t.size(); i++) {
                    const flow_instance_t& f = scenario.active_t.at(i);
                    config_t new_cfg = f.flow_inst->accept(msg, f.cfg);
                    
                    //cout<<endl<< "flow in current : "<<f.flow_inst->get_flow_name()<<cfg_str_c(new_cfg)<<endl<<"f.addr= "<<f.addr<<endl<<endl;
                    
                    if ((new_cfg != null_cfg && f.addr == msg.addr)||(new_cfg != null_cfg && (f.flow_inst->get_index()==4 ||f.flow_inst->get_index()==5) )) {
                        uint32_t flow_index = f.flow_inst->get_index();
                        scenario_t new_scenario = scenario;
                        string cfg_str=cfg_str_c(new_cfg);
                        if(cfg_str=="17")
                        {
                            if(flow_index==0)
                                new_scenario.read0++;
                            else if(flow_index==1)
                                new_scenario.read1++;
                            else if(flow_index==2)
                                new_scenario.write0++;
                            else if(flow_index==3)
                                new_scenario.write1++;
                            else if(flow_index==4)
                                new_scenario.wb0++;
                            else if(flow_index==5)
                                new_scenario.wb1++;
                            new_scenario.order_finish.push_back(flow_index);
                            new_scenario.order_addr.push_back(f.addr);
                            new_scenario.active_t.erase(new_scenario.active_t.begin()+i);
                        }
                        else if(cfg_str =="16"){
                            if(flow_index==0)
                                new_scenario.read0_ch++;
                            else if(flow_index==1)
                                new_scenario.read1_ch++;
                            else if(flow_index==2)
                                new_scenario.write0_ch++;
                            else if(flow_index==3)
                                new_scenario.write1_ch++;
                            new_scenario.order_finish.push_back(flow_index+6);
                            new_scenario.order_addr.push_back(f.addr);
                            new_scenario.active_t.erase(new_scenario.active_t.begin()+i);
                        }
                        else{
                            new_scenario.active_t.at(i).cfg = new_cfg;
                        }
                        match = true;
                        insideflag=true;
                        new_s_stack.push_back(new_scenario);
                        tri_stack.push(tri+1);
                        //cout << "Info: " << msg.toString() << "\t\t (" << f.flow_inst->get_flow_name() << ", " << f.inst_id << ")." << endl << flush;
                        //cout << "+++  new scenario (1) pushed to stack" << endl;
                        //print_scenario(new_scenario);
                    }
                    
                }
                
                // Create a new flow instance to match msg.
                for(uint32_t i=0;i<flow_spec_flag.size();i++){
                    if(flow_spec_flag.at(i)!=99){
                        scenario_t new_scenario = scenario;
                        flow_instance_t new_f;
                        new_f.flow_inst = flow_spec.at(i);
                        ++flow_inst_cnt.at(i);
                        new_f.cfg = flow_spec_flag.at(i);
                        new_f.addr = msg.addr;
                        new_scenario.active_t.push_back(new_f);
                        new_s_stack.push_back(new_scenario);
                        tri_stack.push(tri+1);
                        match = true;
                        insideflag=true;
                    }
                }
       
            }
            if (insideflag==true) {
                s_stack=new_s_stack;
            }
        }
        if (match == false) {
            tri_stack.push(tri+1);
            cout << "Info: " << trace.at(tri).toString() << " not matched, backtrack." << endl;
            pair< vector<scenario_t>,uint32_t> tmp_bad;
            tmp_bad.first=s_stack;
            tmp_bad.second=tri;
            bad_scenario_vec.push_back(tmp_bad);
            break;
            errorfile<<trace.at(tri).toString()<<"line #:"<<tri<<"\n";
            
        }
        else
            s_stack=new_s_stack;
        
        if (s_stack.size()>max)
        {
            max= s_stack.size();
        }
        cout << "======================================" << endl;
    }
    errorfile.close();
    bool succ = false;
    if (s_stack.size() > 0) {
        s_stack=dscen(s_stack);
        for(uint32_t ctt=0;ctt<s_stack.size();ctt++){
            scenario_t good_scen = s_stack.at(ctt);
            if(good_scen.active_t.size()==0){
                cout << endl
                << "***************"<<endl<<
                "  Success -  the scenario that matches all messages is" << endl;
                succ = true;
            }
                cout<<"possiblity #"<<ctt<<endl;
                print_scenario(flow_spec, good_scen);
                cout << endl;
            
            
        }
        
        if (succ == false)
            cout<<"non of the scenarios have finished all its flow"<<endl;
        /**
            for(uint32_t ctt=0;ctt<s_stack.size();ctt++){
                scenario_t good_scen = s_stack.at(ctt);
                cout<<"possiblity #"<<ctt<<endl;
                print_scenario(flow_spec, good_scen);
                cout << endl;
            }
         **/
    }
    
    else if (bad_scenario_vec.size()>0) {
        cout << endl
        << "***  Failed - generating the partial scenarios" << endl;
        pair<vector<scenario_t>,uint32_t> bad_scen= bad_scenario_vec.at(0);
        uint32_t msg_idx = bad_scen.second;
        message_t msg = trace.at(msg_idx);
        cout << "***  the following partial scenario failed to match message (" << msg_idx << ") " << msg.toString() << endl;
        
        for(uint32_t ctt=0;ctt<s_stack.size();ctt++){
            scenario_t tmp_print=bad_scen.first.at(ctt);
            print_scenario(flow_spec, tmp_print);
            cout << endl;}
    }
    getrusage(RUSAGE_SELF, &usage);
    end = usage.ru_stime;
    printf("************************Memory usage = %ld KB\n",
           usage.ru_maxrss);
    printf("************************Time usage: %ld.%ds sec\n", end.tv_sec-start.tv_sec, end.tv_usec-start.tv_usec);
    cout<<"Maximum number of flow instances: "<<max<<endl;
    return 0;
    
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


