//
// Created by guoze.lin on 16/5/6.
//

#ifndef ADCORE_TASK_THREAD_DATA_H
#define ADCORE_TASK_THREAD_DATA_H

#include "protocol/base/abstract_bidding_handler.h"

namespace adservice{
    namespace corelogic{

        typedef std::map<int,protocol::bidding::AbstractBiddingHandler*> BiddingHandlerMap;

        struct BidThreadLocal{
            BiddingHandlerMap biddingHandlers;
            BidThreadLocal(){}
            ~BidThreadLocal(){
                if(!biddingHandlers.empty()){
                    typedef BiddingHandlerMap::iterator Iter;
                    for(Iter iter=biddingHandlers.begin();iter!=biddingHandlers.end();iter++){
                        if(iter->second)
                            delete iter->second;
                    }
                }
            }
        };

        struct TaskThreadLocal{
            int seqId;
            BidThreadLocal bidData;
            TaskThreadLocal():seqId(-1){
            }
            void updateSeqId();
            static void destructor(void* ptr){
                if(ptr){
                    delete ((TaskThreadLocal*)ptr);
                }
            }
        };


    }
}

#endif //ADCORE_TASK_THREAD_DATA_H
