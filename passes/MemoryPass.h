//
// Created by Marcin Bukowiecki on 11.04.2021.
//
#pragma once
#ifndef ROCTESTS_MEMORYPASS_H
#define ROCTESTS_MEMORYPASS_H

#include "../mir/MIR.h"

class HeapAllocatorLifter : public MIRVisitor {
public:

    void visit(MIRInt32Array *mirInt32Array) override;
};


class LabelResolver : public MIRVisitor {
public:
    int labelCounter = 0;

    void visitIf(MIRIf *mirIf) override;

    void visitElse(MIRElse *mirElse) override;
};

#endif //ROCTESTS_MEMORYPASS_H
