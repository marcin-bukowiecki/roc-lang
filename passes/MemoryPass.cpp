//
// Created by Marcin Bukowiecki on 11.04.2021.
//
#include "MemoryPass.h"

void HeapAllocatorLifter::visit(MIRInt32Array *mirInt32Array) {
    if (mirInt32Array->parent->isReturn()) {
        mirInt32Array->allocationSpace = roc::AllocationSpace::HeapAllocation;
    }
    for (auto& m: mirInt32Array->elements) {
        m->accept(this);
    }
}

void LabelResolver::visitIf(MIRIf *mirIf) {
    mirIf->endLabel = new MIRLabel(labelCounter++, "if-start");
    mirIf->endLabel = new MIRLabel(labelCounter++, "if-false");
    mirIf->block->accept(this);
    if (mirIf->hasNextBlock()) {
        mirIf->jumpOver = true;
        mirIf->elseBlock->accept(this);
    }
}

void LabelResolver::visitElse(MIRElse *mirElse) {
    mirElse->endLabel = new MIRLabel(labelCounter++, "else-end");
    if (mirElse->block) mirElse->block->accept(this);
    if (mirElse->ifBlock) mirElse->ifBlock->accept(this);
}


