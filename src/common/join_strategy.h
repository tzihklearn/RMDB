//
// Created by tzih on 24-6-24.
//

#ifndef RMDB_JOIN_STRATEGY_H
#define RMDB_JOIN_STRATEGY_H

class JoinStrategy {
private:
    bool sort_merge;
    int e_id;
public:
    JoinStrategy() : sort_merge(false), e_id(0){}
    void setSortMerge(bool value) {
        sort_merge = value;
    }
    bool getSortMerge() {
        return sort_merge;
    }
    void addEId() {
        ++e_id;
    }
    int getEId() {
        return e_id;
    }
};
#endif //RMDB_JOIN_STRATEGY_H
