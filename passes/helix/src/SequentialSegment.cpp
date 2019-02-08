/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "HELIX.hpp"
#include "SequentialSegment.hpp"
#include "DataFlow.hpp"

using namespace llvm ;

SequentialSegment::SequentialSegment (
  LoopDependenceInfo *LDI, 
  SCCset *sccs,
  int32_t ID
  ){

  /*
   * Set the ID
   */
  this->ID = ID;

  /*
   * Identify all the dependent instructions
   */
  std::set<Instruction *> allInstructionsInSS;
  for (auto scc : *sccs){
    assert(scc->hasCycle());

    /*
     * Add all instructions of the current SCC to the set.
     */
    for (auto node : scc->getNodes()){

      /*
       * Fetch the LLVM value associated to the node.
       */
      auto value = node->getT();

      /*
       * Cast the value to an instruction.
       */
      auto inst = cast<Instruction>(value);

      /*
       * Insert the instruction to the set.
       */
      allInstructionsInSS.insert(inst);
    }
  }

  /*
   * Identify the locations where wait instructions should be placed.
   */
  this->entries = allInstructionsInSS;

  /*
   * Identify the locations where signal instructions should be placed.
   */
  auto dfa = DataFlowAnalysis{};
  auto computeGEN = [](Instruction *i, DataFlowResult *df) {
    auto& gen = df->GEN(i);
    gen.insert(i);
    return ;
  };
  auto computeKILL = [](Instruction *, DataFlowResult *) {
    return ;
  };
  auto computeOUT = [](std::set<Value *>& OUT, Instruction *succ, DataFlowResult *df) {
    auto& inS = df->IN(succ);
    OUT.insert(inS.begin(), inS.end());
    return ;
  } ;
  auto computeIN = [](std::set<Value *>& IN, Instruction *inst, DataFlowResult *df) {
    auto& genI = df->GEN(inst);
    auto& outI = df->OUT(inst);
    IN.insert(outI.begin(), outI.end());
    IN.insert(genI.begin(), genI.end());
    return ;
  };
  dfa.applyBackward(LDI->function, computeGEN, computeKILL, computeIN, computeOUT);

  return ;
}

void SequentialSegment::forEachEntry (
  std::function <void (Instruction *justAfterEntry)> whatToDo){

  /*
   * Iterate over the entries.
   */
  for (auto entry : this->entries){
    whatToDo(entry);
  }

  return ;
}

void SequentialSegment::forEachExit (
  std::function <void (Instruction *justBeforeExit)> whatToDo){

  /*
   * Iterate over the exits.
   */
  for (auto exit : this->exits){
    whatToDo(exit);
  }

  return ;
}

int32_t SequentialSegment::getID (void){
  return this->ID;
}
