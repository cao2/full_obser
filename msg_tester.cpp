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
#include <unistd.h>/* needed to define getpid() */
#include <stdio.h>/* needed for printf() */
#include <pthread.h>
#include "fuc.h"
using namespace std;


int main(int argc, char *argv[]) {
    uint32_t max=0;
   
    vector<message_t> trace;
    cout<<"lala"<<endl;
    string filename(argv[1]);
    
    msgs* readmsg= new msgs;
   
    readmsg->parse(argv[1]);
    trace = readmsg->getMsgs();
    

    
    cout<<"read "<<trace.size()<<endl;
}
