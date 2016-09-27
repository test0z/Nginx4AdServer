//
// Created by guoze.lin on 16/2/3.
//

#ifndef ADCORE_ABSTRACT_SERVICE_H
#define ADCORE_ABSTRACT_SERVICE_H

namespace adservice{
    namespace server{
        class AbstractService{
        public:
            virtual void start() = 0;
        };
    }
}

#endif //ADCORE_ABSTRACT_SERVICE_H
