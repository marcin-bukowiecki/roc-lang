//
// Created by Marcin on 01.05.2021.
//
#pragma once
#ifndef ROCTESTS_CONSTANTS_H
#define ROCTESTS_CONSTANTS_H

#include <string>

namespace roc {

    namespace methods {

        //method ids
        static long long toStringMethodId = 0;
        static long long typeIdMethodId = 1;
        static long long hashCodeMethodId = 2;
        static long long equalsMethodId = 3;

        static long long getMethodId(const std::string& name) {
            if (name == "toString") {
                return toStringMethodId;
            } else if (name == "typeId") {
                return typeIdMethodId;
            } else if (name == "hashCode") {
                return hashCodeMethodId;
            } else if (name == "equals") {
                return equalsMethodId;
            } else {
                throw ("Undefined method: " + name).c_str();
            }
        }
    }
}

#endif //ROCTESTS_CONSTANTS_H
