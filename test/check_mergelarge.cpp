#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "logstore.h"
#include "datapage.h"
#include "logiterators.h"
#include "merger.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#undef begin
#undef end

#include "check_util.h"

void insertProbeIter(int  NUM_ENTRIES)
{
    srand(1000);
    unlink("storefile.txt");
    unlink("logfile.txt");

    sync();

    //data generation
//    std::vector<std::string> * data_arr = new std::vector<std::string>;
    std::vector<std::string> * key_arr = new std::vector<std::string>;
    
//    preprandstr(NUM_ENTRIES, data_arr, 10*8192);
    preprandstr(NUM_ENTRIES+200, key_arr, 100, true);
    
    std::sort(key_arr->begin(), key_arr->end(), &mycmp);

    removeduplicates(key_arr);
    if(key_arr->size() > NUM_ENTRIES)
        key_arr->erase(key_arr->begin()+NUM_ENTRIES, key_arr->end());
    
    NUM_ENTRIES=key_arr->size();
    
    bufferManagerNonBlockingSlowHandleType = IO_HANDLE_PFILE;

    Tinit();

    int xid = Tbegin();

    merge_scheduler mscheduler;    
    logtable ltable;

    int pcount = 100;
    ltable.set_fixed_page_count(pcount);

    recordid table_root = ltable.allocTable(xid);

    Tcommit(xid);
    
    //xid = Tbegin();

    int lindex = mscheduler.addlogtable(&ltable);
    ltable.setMergeData(mscheduler.getMergeData(lindex));
    
    mscheduler.startlogtable(lindex);

    printf("Stage 1: Writing %d keys\n", NUM_ENTRIES);
    
    struct timeval start_tv, stop_tv, ti_st, ti_end;
    double insert_time = 0;
    int dpages = 0;
    int npages = 0;
    DataPage<datatuple> *dp=0;
    int64_t datasize = 0;
    std::vector<pageid_t> dsp;
    gettimeofday(&start_tv,0);
    for(int i = 0; i < NUM_ENTRIES; i++)
    {
        //prepare the key
        datatuple newtuple;        
        uint32_t keylen = (*key_arr)[i].length()+1;
        newtuple.keylen = &keylen;
        
        newtuple.key = (datatuple::key_t) malloc(keylen);
        memcpy((byte*)newtuple.key, (*key_arr)[i].c_str(), keylen);
        //for(int j=0; j<keylen-1; j++)
        //    newtuple.key[j] = (*key_arr)[i][j];
        //newtuple.key[keylen-1]='\0';

        //prepare the data
        std::string ditem;
        getnextdata(ditem, 10*8192);
        uint32_t datalen = ditem.length()+1;
        newtuple.datalen = &datalen;        
        newtuple.data = (datatuple::data_t) malloc(datalen);
        memcpy((byte*)newtuple.data, ditem.c_str(), datalen);
//        for(int j=0; j<datalen-1; j++)
//            newtuple.data[j] = (*data_arr)[i][j];
//        newtuple.data[datalen-1]='\0';        
        
        /*
        printf("key: \t, keylen: %u\ndata:  datalen: %u\n",
               //newtuple.key,
               *newtuple.keylen,
               //newtuple.data,
               *newtuple.datalen);
               */
        
        datasize += newtuple.byte_length();

        gettimeofday(&ti_st,0);        
        ltable.insertTuple(newtuple);
        gettimeofday(&ti_end,0);
        insert_time += tv_to_double(ti_end) - tv_to_double(ti_st);

        free(newtuple.key);
        free(newtuple.data);
        
    }
    gettimeofday(&stop_tv,0);
    printf("insert time: %6.1f\n", insert_time);
    printf("insert time: %6.1f\n", (tv_to_double(stop_tv) - tv_to_double(start_tv)));

    printf("\nTREE STRUCTURE\n");
    //ltable.get_tree_c1()->print_tree(xid);
    printf("datasize: %lld\n", datasize);
    //sleep(20);

    /*
    //Tcommit(xid);
    xid = Tbegin();


    printf("Stage 2: Looking up %d keys:\n", NUM_ENTRIES);

    int found_tuples=0;
    for(int i=NUM_ENTRIES-1; i>=0; i--)
    {        
        int ri = i;
        //printf("key index%d\n", i);
        fflush(stdout);

        //get the key
        uint32_t keylen = (*key_arr)[ri].length()+1;        
        datatuple::key_t rkey = (datatuple::key_t) malloc(keylen);
        memcpy((byte*)rkey, (*key_arr)[ri].c_str(), keylen);
        //for(int j=0; j<keylen-1; j++)
        //rkey[j] = (*key_arr)[ri][j];
        //rkey[keylen-1]='\0';

        //find the key with the given tuple
        datatuple *dt = ltable.findTuple(xid, rkey, keylen);

        assert(dt!=0);
        //if(dt!=0)
        {
        found_tuples++;
        assert(*(dt->keylen) == (*key_arr)[ri].length()+1);
        //assert(*(dt->datalen) == (*data_arr)[ri].length()+1);
        free(dt->keylen);
        free(dt);
        }
        dt = 0;
        free(rkey);
    }
    printf("found %d\n", found_tuples);

    key_arr->clear();
    //data_arr->clear();
    delete key_arr;
    //delete data_arr;
    */
    
    mscheduler.shutdown();
    printf("merge threads finished.\n");
    gettimeofday(&stop_tv,0);
    printf("run time: %6.1f\n", (tv_to_double(stop_tv) - tv_to_double(start_tv)));
    
    //Tcommit(xid);
    
    Tdeinit();
    
    
}



/** @test
 */
int main()
{
    insertProbeIter(25000);

    
    
    return 0;
}
