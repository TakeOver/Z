#pragma once
#include <set>
namespace Z{
        class Collectable {
        public:
                virtual ~Collectable(){}
                virtual void MarkChilds(std::set<Collectable*>&) = 0;
        };
}