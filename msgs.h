//
//  msgs.cpp
//  
//
//  Created by Yuting Cao on 4/1/17.
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
        uint32_t srcs[42];
        uint32_t dests[42];
        uint32_t lengths[42];
        uint32_t num_sigs=42;
        srcs[0]=cpu0;       dests[0]=cache0;        lengths[0]=73; //cpu_req1
        srcs[1]=cache0;     dests[1]=cpu0;          lengths[1]=73;//cpu_res1
        srcs[2]=cpu1;       dests[2]=cache1;        lengths[2]=73;//cpu_req2
        srcs[3]=cache1;     dests[3]=cpu1;          lengths[3]=73;//cpu_res2
        
        srcs[4]=cache1;     dests[4]=cache0;        lengths[4]=73;//snp_req1
        srcs[5]=cache0;     dests[5]=cache1;        lengths[5]=73;//snp_res1
        srcs[6]=cache0;     dests[6]=cache1;        lengths[6]=1;//snp_hit1
        
        srcs[7]=cache0;     dests[7]=cache1;        lengths[7]=73;//snp_req2
        srcs[8]=cache1;     dests[8]=cache0;        lengths[8]=73;//snp_res2
        srcs[9]=cache1;     dests[9]=cache0;        lengths[9]=1;//snp_hit2
        
        srcs[10]=membus;    dests[10]=cache0;       lengths[10]=76; //up_snp_req
        srcs[11]=cache0;    dests[11]=membus;       lengths[11]=76;//up_snp_res
        srcs[12]=cache0;    dests[12]=membus;       lengths[12]=1;//up_snp_hit
        
        srcs[13]=cache0;    dests[13]=membus;       lengths[13]=73;//bus_req1
        srcs[14]=membus;    dests[14]=cache0;       lengths[14]=553;//bus_res1
        
        srcs[15]=cache1;    dests[15]=membus;       lengths[15]=73;//bus_req2
        srcs[16]=membus;    dests[16]=cache1;       lengths[16]=553;//bus_res2
        
        srcs[17]=membus;    dests[17]=mem;          lengths[17]=1;//rvalid
        srcs[18]=mem;       dests[18]=membus;       lengths[18]=1;//rres
        srcs[19]=membus;    dests[19]=mem;          lengths[19]=1;//wvalid
        srcs[20]=membus;    dests[20]=mem;          lengths[20]=32;//waddr
        srcs[21]=mem;       dests[21]=membus;       lengths[21]=1;//wrsp
        
        
        srcs[22]=membus;    dests[22]=gfx;          lengths[22]=1;//rvalid_gfx
        srcs[23]=gfx;       dests[23]=membus;       lengths[23]=1;//rres_gfx
        srcs[24]=membus;    dests[24]=gfx;          lengths[24]=1;//wvalid_gfx
        srcs[25]=membus;    dests[25]=gfx;          lengths[25]=32;//waddr_gfx
        srcs[26]=gfx;       dests[26]=membus;       lengths[26]=1;//wrsp_gfx
        
        srcs[27]=membus;    dests[27]=uart;         lengths[27]=1;//rvalid_uart
        srcs[28]=uart;      dests[28]=membus;       lengths[28]=1;//rres_uart
        srcs[29]=membus;    dests[29]=uart;         lengths[29]=1;//wvalid_uart
        srcs[30]=membus;    dests[30]=uart;         lengths[30]=32; //waddr_uart
        srcs[31]=uart;      dests[31]=membus;       lengths[31]=1;//wrsp_uart
        
        srcs[32]=membus;    dests[32]=usb;          lengths[32]=1;//rvalid_usb
        srcs[33]=usb;       dests[33]=membus;       lengths[33]=1;//rres_usb
        srcs[34]=membus;    dests[34]=usb;          lengths[34]=1;//wvalid_usb
        srcs[35]=membus;    dests[35]=usb;          lengths[35]=32;//waddr_usb
        srcs[36]=usb;       dests[36]=membus;       lengths[36]=1;//wrsp_usb
        
        srcs[37]=membus;    dests[37]=audio;        lengths[37]=1;//rvalid_audio
        srcs[38]=audio;     dests[38]=membus;       lengths[38]=1;//rres_audio
        srcs[39]=membus;    dests[39]=audio;        lengths[39]=1;//wvalid_audio
        srcs[40]=membus;    dests[40]=audio;        lengths[40]=32;//waddr_audio
        srcs[41]=audio;     dests[41]=membus;       lengths[41]=1;//wrsp_audio


      
        ifstream trace_file(filename);
        ofstream msg_file;
        msg_file.open ("msgs.txt",ios::trunc);
        if (trace_file.is_open()) {
            std::string line;
            message_t new_msg;
            int pl[42];
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
                string tmp_str = line.substr(0, lengths[0]);
                if (tmp_str.at(0)=='1'){
                    new_msg.src = cpu0;
                    new_msg.dest = cache0;
                    //new_msg //
                    if (tmp_str.substr(1,8)=="01000000")
                        new_msg.cmd = rd;
                    else if (tmp_str.substr(1,8)=="11000000")
                        new_msg.cmd =pwr;
                    else if (tmp_str.substr(1,8)=="10000000")
                        new_msg.cmd = wt;
                    else
                        cout<<"cmd doesn't looks right. Error 1"<<endl;
                    new_msg.addr = stol(tmp_str.substr(8,32));
                    //new_msg.addr=0;
                    msg_file<<new_msg.toString()<<"\n";
                    trace.push_back(new_msg);
                    cout<<0<<": "<<new_msg.toString()<<endl;
                    num++;
                }
                cout<<"pass 2"<<endl;
                for (j=1; j<num_sigs; j++) {
                    tmp_str = line.substr(pl[j-1]+1,lengths[j]);
                    cout<<j<<":"<<tmp_str<<endl;
                    //cout<<"start:"<<j<<":"<<tmp_str<<endl;
                    if (lengths[j]>73 )
                        tmp_str = tmp_str.substr(3,73);
                    if (lengths[j]>=73 && tmp_str.at(0)=='1'){
                       
                        new_msg.src = srcs[j];
                        new_msg.dest = dests[j];

                        //new_msg //
                        if (j==10 ||j ==11||j==4||j==5||j==7||j==8)
                            new_msg.cmd=snp;
                        else if (tmp_str.substr(1,8)=="01000000")
                            new_msg.cmd = rd;
                        else if (tmp_str.substr(1,8)=="11000000")
                            new_msg.cmd =pwr;
                        else if (tmp_str.substr(1,8)=="10000000")
                            new_msg.cmd = wt;
                        else
                            cout<<"cmd doesn't looks right. Error 1"<<endl;
                        new_msg.addr = stol(tmp_str.substr(8,32));
                        //new_msg.addr=0;
                        msg_file<<new_msg.toString()<<"\n";
                        trace.push_back(new_msg);
                        num++;
                        cout<<j<<": "<<new_msg.toString()<<endl;
                    }
                    else if (lengths[j]==1 and tmp_str.at(0)=='1' and j!=6 and j!=9 and j!=12){
                        new_msg.src=srcs[j];
                        new_msg.dest=dests[j];
                        if (j==17 ||j==18|| j==22||j==23 ||j==27||j==28 ||j==32||j==33||j==37||j==38)
                            new_msg.cmd = rd;
                        else{
                            new_msg.cmd = wt;
                            if (j==19 ||j==23 || j==29 ||j==34 || j==39){
                                new_msg.addr=stol(line.substr(pl[j]+1,32));
                            }
                            j++;
                        }
                        msg_file<<new_msg.toString()<<"\n";
                        trace.push_back(new_msg);
                        num++;
                        cout<<j<<": "<<new_msg.toString()<<endl;
                    }
                    
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
