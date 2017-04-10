#include <stdlib.h>/* needed to define exit() */
#include <unistd.h>/* needed to define getpid() */
#include <stdio.h>/* needed for printf() */
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
#include "msgs.h"
#include "vcd_msg.h"
#include <sys/time.h>
#include <sys/resource.h>
#include "fuc.h"
#define CHECK_MEM

//uint32_t num_flow=36;

using namespace std;

//vector<message_t> uniq_msg;
set<message_t> uniq_msg;
void uniq(lpn_t* protocol){/**
    ofstream uniq_msg;
    uniq_msg.open("uniq_msg.txt", ios::trunc);
    uniq_msg.close();**/
    for (message_t x: protocol->msg_vector){
//        flag=false;
//        for (message_t y: uniq_msg)
//            if (y.src==x.src && y.dest==x.dest && y.cmd==x.cmd){
//                flag=true;
//                break;
//            }
//        if (flag==false)
//            uniq_msg.push_back(x);
        uniq_msg.insert(x);
    }
}

int main(int argc, char *argv[]) {
    init();
    unsigned int pid = getpid();

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
    uniq(cpu0_read);
    uniq(cpu1_read);
    
    uniq(cpu0_write);
    uniq(cpu1_write);
    uniq(write_back0);
    uniq(write_back1);
    lpn_t* cpu0_read_audio= build_cpu0_read_audio();
    lpn_t* cpu0_write_audio= build_cpu0_write_audio();
    lpn_t* cpu1_read_audio= build_cpu1_read_audio();
    lpn_t* cpu1_write_audio= build_cpu1_write_audio();
    uniq(cpu0_read_audio);
    uniq(cpu0_write_audio);
    uniq(cpu1_read_audio);
    
    uniq(cpu1_write_audio);
    lpn_t* cpu0_read_gfx= build_cpu0_read_gfx();
    lpn_t* cpu0_write_gfx= build_cpu0_write_gfx();
    lpn_t* cpu1_read_gfx= build_cpu1_read_gfx();
    lpn_t* cpu1_write_gfx= build_cpu1_write_gfx();
    uniq(cpu0_read_gfx);
    uniq(cpu0_write_gfx);
    uniq(cpu1_read_gfx);
    uniq(cpu1_write_gfx);
    
    lpn_t* cpu0_read_usb= build_cpu0_read_usb();
    lpn_t* cpu0_write_usb= build_cpu0_write_usb();
    lpn_t* cpu1_read_usb= build_cpu1_read_usb();
    lpn_t* cpu1_write_usb= build_cpu1_write_usb();
    uniq(cpu0_read_usb);
    uniq(cpu0_write_usb);
    uniq(cpu1_read_usb);
    uniq(cpu1_write_usb);
    lpn_t* cpu0_read_uart= build_cpu0_read_uart();
    lpn_t* cpu0_write_uart= build_cpu0_write_uart();
    lpn_t* cpu1_read_uart= build_cpu1_read_uart();
    lpn_t* cpu1_write_uart= build_cpu1_write_uart();
    uniq(cpu0_read_uart);
    
    uniq(cpu0_write_uart);
    uniq(cpu1_read_uart);
    uniq(cpu1_write_uart);
    lpn_t* poweron_uart=build_poweron_uart();
    lpn_t* poweroff_uart=build_poweroff_uart();
    uniq(poweron_uart);
    uniq(poweroff_uart);
    
    lpn_t* poweron_gfx=build_poweron_gfx();
    lpn_t* poweroff_gfx=build_poweroff_gfx();
    uniq(poweron_gfx);
    uniq(poweroff_gfx);
         
    lpn_t* poweron_usb=build_poweron_usb();
    lpn_t* poweroff_usb=build_poweroff_usb();
    uniq(poweron_usb);
    uniq(poweroff_usb);
    lpn_t* poweron_audio=build_poweron_audio();
    lpn_t* poweroff_audio=build_poweroff_audio();
    uniq(poweron_audio);
    uniq(poweroff_audio);
    
         lpn_t* gfx_upstream_write=build_gfx_upstream_write();
         lpn_t* gfx_upstream_read=build_gfx_upstream_read();
         lpn_t* audio_upstream_write=build_audio_upstream_write();
         lpn_t* audio_upstream_read=build_audio_upstream_read();
         lpn_t* usb_upstream_read=build_usb_upstream_read();
         lpn_t* uart_upstream_read=build_uart_upstream_read();
         uniq(gfx_upstream_read);
         uniq(gfx_upstream_write);
         uniq(audio_upstream_read);
         uniq(audio_upstream_write);
         uniq(usb_upstream_read);
         uniq(uart_upstream_read);
    
    //sort(uniq_msg.begin(),uniq_msg.end());
    //uniq_msg.erase( unique( uniq_msg.begin(), uniq_msg.end() ), uniq_msg.end() );
    
    //get the unique mesgs, USEFULLLL!!!!!!!!!!!!!
    /**
    cout<<"unique messages:"<<endl;
    ofstream uni_msg;
    uni_msg.open("uniq_msgs.txt",ios::trunc);
    for(message_t x: uniq_msg){
        uni_msg<<x.toString()<<"\n";
        cout<<x.toString()<<endl;}
    uni_msg.close();
    **/
    
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
    
    flow_spec.push_back(cpu0_read_audio);
    cpu0_read_audio->set_index(6);
    flow_spec.push_back(cpu0_write_audio);
    cpu0_write_audio->set_index(7);
    flow_spec.push_back(cpu1_read_audio);
    cpu1_read_audio->set_index(8);
    flow_spec.push_back(cpu1_write_audio);
    cpu1_write_audio->set_index(9);
    
    flow_spec.push_back(cpu0_read_gfx);
    cpu0_read_gfx->set_index(10);
    flow_spec.push_back(cpu0_write_gfx);
    cpu0_write_gfx->set_index(11);
    flow_spec.push_back(cpu1_read_gfx);
    cpu1_read_gfx->set_index(12);
    flow_spec.push_back(cpu1_write_gfx);
    cpu1_write_gfx->set_index(13);

    
    flow_spec.push_back(cpu0_read_usb);
    cpu0_read_usb->set_index(14);
    flow_spec.push_back(cpu0_write_usb);
    cpu0_write_usb->set_index(15);
    flow_spec.push_back(cpu1_read_usb);
    cpu1_read_usb->set_index(16);
    flow_spec.push_back(cpu1_write_usb);
    cpu1_write_usb->set_index(17);
    
    
    flow_spec.push_back(cpu0_read_uart);
    cpu0_read_uart->set_index(18);
    flow_spec.push_back(cpu0_write_uart);
    cpu0_write_uart->set_index(19);
    flow_spec.push_back(cpu1_read_uart);
    cpu1_read_uart->set_index(20);
    flow_spec.push_back(cpu1_write_uart);
    cpu1_write_uart->set_index(21);
    
    flow_spec.push_back(poweron_uart);
    poweron_uart->set_index(22);
    flow_spec.push_back(poweroff_uart);
    poweroff_uart->set_index(23);
    
    flow_spec.push_back(poweron_usb);
    poweron_usb->set_index(24);
    flow_spec.push_back(poweroff_usb);
    poweroff_usb->set_index(25);
    
    flow_spec.push_back(poweron_audio);
    poweron_audio->set_index(26);
    flow_spec.push_back(poweroff_audio);
    poweroff_audio->set_index(27);
    
    flow_spec.push_back(poweron_gfx);
    poweron_gfx->set_index(28);
    flow_spec.push_back(poweroff_gfx);
    poweroff_gfx->set_index(29);
    
    flow_spec.push_back(gfx_upstream_write);
    gfx_upstream_write->set_index(30);
    flow_spec.push_back(gfx_upstream_read);
    gfx_upstream_read->set_index(31);
    
    flow_spec.push_back(audio_upstream_write);
    audio_upstream_write->set_index(32);
    flow_spec.push_back(audio_upstream_read);
    audio_upstream_read->set_index(33);
    
    flow_spec.push_back(usb_upstream_read);
    usb_upstream_read->set_index(34);
    flow_spec.push_back(uart_upstream_read);
    uart_upstream_read->set_index(35);
         
    vector<uint32_t> flow_inst_cnt;
    for(uint32_t ii=0;ii<36;ii++){
        flow_inst_cnt.push_back(0);}
   
    
    
    ofstream errorfile;
    errorfile.open ("erromsg.txt",ios::trunc);
    
    vector<message_t> trace;
    //cout<<"lala"<<endl;
    string filename(argv[1]);
 /**
//    if (filename.substr(filename.size()-3).compare("vcd")==0){
//        vcd_msg* readmsg= new vcd_msg;
//        readmsg->parse(argv[1]);
//        trace = readmsg->getMsgs();
//    
//    }
//    else{
//        msgs* readmsg= new msgs;
//        readmsg->parse(argv[1]);
//        trace = readmsg->getMsgs();
    //}
**/
    ifstream trace_file(argv[1]);
    //ofstream msg_file;
    //msg_file.open ("msgs.txt",ios::trunc);
    if (trace_file.is_open()) {
        std::string line;
        vector<scenario_t> s_stack;
        s_stack.push_back(scenario_t());
        uint32_t tim=0;
        vector<pair< vector<scenario_t>,uint32_t> >  bad_scenario_vec;
        while (getline(trace_file, line)){
            tim++;
            stack<uint32_t> tri_stack;
            tri_stack.push(0);
            
            msgs* readmsg=new msgs;
            readmsg->parse(line);
            trace=readmsg->getMsgs();
            bool match = true;
            bool flag =false;
            while (tri_stack.size() != 0) {
                match=false;
                uint32_t tri = tri_stack.top();
                tri_stack.pop();
                
                // If index tri reaches the end of trace, store the current scenario.
                if (tri == trace.size()) {
                    //break if a scenario is found to match all messages.
                    break;
                }
                
                //if((tri%5==0 &&s_stack.size()>10)|| s_stack.size()>tri*tri*2){
                if((tri%50==0 &&s_stack.size()>50)|| s_stack.size()>(tri*tri*2+10)){
                    cout<<"************************"<<endl;
                    cout<<"dscen called, orig size "<<s_stack.size()<<endl;
                    s_stack=dscen(s_stack);
                    cout<<"************************"<<endl;
                    cout<<"NEW size: "<<s_stack.size()<<endl;
                    
                }
//#ifdef CHECK_MEM
//                if (tri%100==0){
//                    getMemUsage(pid, argv[2]);
//                }
//#endif
                vector<scenario_t> tmp_s_stack=s_stack;
                message_t msg(trace.at(tri));
                vector<scenario_t> new_s_stack;
                cout << tim
                <<"***  " << msg.toString() <<"  "<<s_stack.size() <<endl << endl;
                vector<uint32_t> flow_spec_flag;
                //find out if new msg can create a new flow_inst
                for (uint32_t i = 0; i < flow_spec.size(); i++) {
                    //cout<<"flow "<<i;
                    lpn_t* f = flow_spec.at(i);
                    config_t new_cfg = f->accept(msg);
                    if (new_cfg != null_cfg){
                        cout<<" matched new flow "<<f->get_flow_name()<<endl;
                        flow_spec_flag.push_back(new_cfg);}
                    else
                        flow_spec_flag.push_back(99);
                    //cout<<endl;
                }
                
               
                // Match the next message from trace against the current scenario.
                //cout<<"stack size: "<<s_stack.size()<<endl;
                for(uint32_t ct=0;ct<s_stack.size();ct++)
                {
                    scenario_t scenario = s_stack.at(ct);
                
                    for (uint32_t i = 0; i < scenario.active_t.size(); i++) {
                        const flow_instance_t& f = scenario.active_t.at(i);
                        config_t new_cfg = f.flow_inst->accept(msg, f.cfg);
                        
                        if ((new_cfg != null_cfg && f.addr == msg.addr && f.time!=tim)||(new_cfg != null_cfg )) {
                            
                            uint32_t flow_index = f.flow_inst->get_index();
                            scenario_t new_scenario = scenario;
                            string cfg_str=cfg_str_c(new_cfg);
                            if(cfg_str=="17")
                            {
                                //cout<<"finished "<<f.flow_inst->get_flow_name()<<endl;
                                
                                new_scenario.finished.at(flow_index)=new_scenario.finished.at(flow_index)+1;
                                //cout<<"1"<<endl;
                                order_inst end;
                                end.start=false;
                                end.addr=f.addr;
                                end.flow=flow_index;
                                end.time=tim;
                                new_scenario.order.push_back(end);
                                new_scenario.active_t.erase(new_scenario.active_t.begin()+i);
                            }
                            else if(cfg_str =="16"){
                                //cout<<"finished "<<f.flow_inst->get_flow_name()<<endl;

                                new_scenario.finished.at(flow_index+num_flow)=new_scenario.finished.at(flow_index+num_flow)+1;
                                //cout<<"3"<<endl;
                                order_inst end;
                                end.start=false;
                                end.addr=f.addr;
                                end.flow=flow_index+6;
                                end.time=tim;
                                new_scenario.order.push_back(end);
                                
                                new_scenario.active_t.erase(new_scenario.active_t.begin()+i);
                            }
                            else{
                                new_scenario.active_t.at(i).cfg = new_cfg;
                                new_scenario.active_t.at(i).time= tim;
                            }
                            //cout<<"2"<<endl;
                            match = true;
                            //cout<<"matched"<<endl;
                            new_s_stack.push_back(new_scenario);
                            tri_stack.push(tri+1);
                            //cout << "Info: " << msg.toString() << "\t\t (" << f.flow_inst->get_flow_name() << ", " << f.inst_id << ")." << endl << flush;
                            //cout << "+++  new scenario (1) pushed to stack" << endl;
                            //print_scenario(new_scenario);
                        }
                        
                    }
                    
                    
                    // Create a new flow instance to match msg.
                    for(uint32_t i=0;i<flow_spec_flag.size();i++){
                       // cout<<i<<":"<<flow_spec_flag.at(i)<<endl;
                        if(flow_spec_flag.at(i)!=99){
                            scenario_t new_scenario = scenario;
                            flow_instance_t new_f;
                            new_f.flow_inst = flow_spec.at(i);
                            ++flow_inst_cnt.at(i);
                            new_f.cfg = flow_spec_flag.at(i);
                            new_f.addr = msg.addr;
                            new_f.time=tim;
                            new_scenario.active_t.push_back(new_f);
                            
                            tri_stack.push(tri+1);
                            match = true;
                            //cout<<"new flow: "<< flow_spec.at(i)->get_flow_name() << "adds: "<< new_f.addr<<endl;
                            order_inst end;
                            end.start=true;
                            end.addr=msg.addr;
                            end.flow=i;
                            end.time=tim;
                            new_scenario.order.push_back(end);
                            //cout<<"active : size"<<new_scenario.active_t.size()<<endl;
                            //cout<<"stack size: "<<new_s_stack.size()<<endl;
                            new_s_stack.push_back(new_scenario);
                        }
                    }
                    //cout<<"4"<<endl;
                    
                    
                }
                
                if (match == false) {
                    flag=true;
                    tri_stack.push(tri+1);
                    cout << "Info: " << trace.at(tri).toString() << " not matched, backtrack." << endl;
                    pair< vector<scenario_t>,uint32_t> tmp_bad;
                    tmp_bad.first=tmp_s_stack;
                    tmp_bad.second=tri;
                    bad_scenario_vec.push_back(tmp_bad);
                    break;
                    errorfile<<trace.at(tri).toString()<<"line #:"<<tri<<"\n";
                    
                }
                else{
                    s_stack=new_s_stack;
                }
                if (s_stack.size()>max)
                {
                    max= s_stack.size();
                }
                cout << "======================================" << endl;
            }
            if (flag==true){
                
                break;
            }
        }
        errorfile.close();
        bool succ = false;
        cout<<"==============================================="<<endl;
        cout<<"==============================================="<<endl;
        cout<<"==============================================="<<endl;
        cout<<"==============================================="<<endl;
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
        //    unsigned int pid = getpid();
        getMemUsage(pid, argv[2]);
        max_mem(argv[2]);
    }
   
    
    return 0;
    
}


