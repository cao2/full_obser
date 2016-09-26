//
//  msgs.cpp
//  
//
//  Created by Yuting Cao on 4/7/16.
//
//

#include "msg_def.h"
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <fstream>

class msgs{


private:
    vector<message_t> trace;
public:
    msgs() {}
    
    void parse(char *filename){
        uint32_t srcs[18]={ cpu0 ,      cpu1 ,      cache0 ,    cache1 ,    cache0 ,    cache1 ,    membus ,    membus ,    cache0 ,    cache1 , membus , membus , cache0 , cache1 , membus , mem , membus,mem };
        uint32_t dests[18]={ cache0 ,   cache1 ,    cpu0 ,      cpu1 ,      membus ,    membus ,    cache0 ,    cache1 ,    membus ,    membus , cache0 , cache1 , membus , membus , mem , membus , mem ,membus};
        // Open input trace file

        
        ifstream trace_file(filename);
        ofstream msg_file;
        msg_file.open ("msgs.txt",ios::trunc);
        
        if (trace_file.is_open()) {
            std::string line;
            message_t new_msg;
            int pl[17];
            int linenm=0;
            int num =0;
            while (getline(trace_file, line)){
                // From each line, get the message information to create a new msg.
                linenm++;
                uint32_t state=0;
                
                for (uint32_t i = 0; i < line.size(); i++)
                    if (line.at(i) == ',') {
                        pl[state] = i;
                        state++;
                    }
                //cout<<endl<<line<<endl;
                uint32_t j =0;
                string tmp_str = line.substr(0, 52);
                if (tmp_str.at(0)=='1'){
                    new_msg.src = cpu0;
                    new_msg.dest = cache0;
                    //new_msg //
                    if (tmp_str.substr(1,2)=="10")
                        new_msg.cmd = rd;
                    else if (tmp_str.substr(1,2)=="11")
                        new_msg.cmd =pwr;
                    else
                        new_msg.cmd = wt;
                    new_msg.addr = stol(tmp_str.substr(3,6));
                    //new_msg.addr=0;
                    msg_file<<new_msg.toString()<<"\n";
                    trace.push_back(new_msg);
                    //cout<<new_msg.toString()<<endl;

                    num++;
                }
                
                for (j=1; j<17; j++) {
                    tmp_str = line.substr(pl[j-1]+2,52);
                    //cout<<"start:"<<j<<":"<<tmp_str<<endl;
                    if (j==14){
                        if (tmp_str.at(0)=='0')
                            new_msg.src = bus0;
                        else
                            new_msg.src = bus1;
                        tmp_str = tmp_str.substr(1,52);
                        
                    }
                    else if (j==15){
                        if (tmp_str.at(0)=='0')
                            new_msg.dest = bus0;
                        else
                            new_msg.dest = bus1;
                        tmp_str = tmp_str.substr(1,52);

                    }
                    if (tmp_str.at(0)=='1'){
                        //cout<<"inside : "<<tmp_str<<endl;
                        if (j!=14) {
                            new_msg.src = srcs[j];
                        }
                        if (j!=15) {
                            new_msg.dest= dests[j];

                        }
                        if (j==10 or j==11 or j==12 or j==13)
                            new_msg.cmd=snp;
                        else if (j==8 or j==9 or j==16 )
                            new_msg.cmd=wb;
                        else if(tmp_str.substr(1,2)=="10")
                            new_msg.cmd=rd;
                        else if(tmp_str.substr(1,2)=="11")
                            new_msg.cmd=pwr;
                        else
                            new_msg.cmd=wt;
                        //write to msg file
                        msg_file<<new_msg.toString()<<"\n";
                        new_msg.addr = stol(tmp_str.substr(3,6));
                        //new_msg.addr=0;
                        trace.push_back(new_msg);
                        //cout<<new_msg.toString()<<endl;
                        num++;
                    }
                }
                
                tmp_str = line.substr(pl[16]+2,1);
                if (tmp_str.at(0)=='1') {
                    new_msg.src = srcs[17];
                    new_msg.dest = dests[17];
                    new_msg.cmd = wb;
                    msg_file<<new_msg.toString()<<"\n";
                    new_msg.addr = stol(tmp_str.substr(3,6));
                    //new_msg.addr=0;
                    trace.push_back(new_msg);
                    //cout<<new_msg.toString()<<endl;

                    num++;

                }
            }
            cout<<"finished"<<endl;
            trace_file.close();
            msg_file.close();
            
        }
        else {
            cout << "Unable to open file" << endl;
        }
        cout << "Info: read " << trace.size() << " messages." << endl;
    }
    
    vector<message_t> getMsgs(){
        return trace;
    }
};