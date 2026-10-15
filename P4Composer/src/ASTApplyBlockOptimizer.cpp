#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/ASTApplyBlockOptimizer.h"

void PrintBoolVariables(std::unordered_map<std::string, OptimizerBoolVariableValue> *boolVariables){
    for(auto it = boolVariables->begin(); it != boolVariables->end(); it++){
        printf("%s: %s\n", it->first.c_str(), ASTApplyBlockOptimizer::OBVVToString(it->second).c_str());
    }
}

ASTApplyBlockOptimizer::ASTApplyBlockOptimizer(){
}

ASTApplyBlockOptimizer::~ASTApplyBlockOptimizer(){
}

std::string ASTApplyBlockOptimizer::OBVVToString(OptimizerBoolVariableValue obvv){
    switch(obvv){
        case OPTIMIZER_BOOL_VARIABLE_FALSE:
            return "false";
        case OPTIMIZER_BOOL_VARIABLE_TRUE:
            return "true";
        case OPTIMIZER_BOOL_VARIABLE_UNKNOWN:
            return "unknown";
    }

    Assert(false);
    return "";
}

void ASTApplyBlockOptimizer::Optimize(AST *applyBlock){
    //return; //Temp

    if(applyBlock == NULL)
        return;

    Assert(applyBlock->getNodeType() == NT_STAT_OR_DEC_LIST);

    bool changed = true;
    while(changed){
        changed = false;

        applyBlock->RemoveEmptySwitches();
        ASTApplyBlockOptimizer::RemoveUnusedVariables(applyBlock);
        std::unordered_map<std::string, OptimizerBoolVariableValue> boolVariables;
        ASTApplyBlockOptimizer::OptimizeBlock(applyBlock, &boolVariables);
        ASTApplyBlockOptimizer::RemoveUnusedVariables(applyBlock);

        applyBlock->RemoveEmptyIfElses();
        std::set<std::string> boolVariablesSetTrueOnce = ASTApplyBlockOptimizer::GetVariablesSetTrueOnce(applyBlock);
        std::vector<OptimizerStatementSuccessors> successorGraph = ASTApplyBlockOptimizer::GetSucessorGraph(applyBlock); 
        
        /*
        for(OptimizerStatementSuccessors s : successorGraph){
            printf("Statement: %p - \n", s.statement);
            s.statement->writeCode(stdout);
            for(AST *n : s.nextStatements){
                printf("  Next: %p\n", n);
            }
        }
        GenStatementSuccessorsDot(successorGraph, "successors.dot");
        exit(1);
        */

        std::unordered_set<AST*> toDelete;
        changed = changed || ASTApplyBlockOptimizer::SimplifyTableApplyIfs(applyBlock, boolVariablesSetTrueOnce, successorGraph, &toDelete);
        changed = changed || ASTApplyBlockOptimizer::SimplifyAdjacentIfs(applyBlock);
        ASTApplyBlockOptimizer::RemoveUnusedVariables(applyBlock);
        changed = changed || ASTApplyBlockOptimizer::SimplifySimilarSwitchCases(applyBlock);

        changed = changed || ASTApplyBlockOptimizer::SimplifySubsequentIfsFromVars(applyBlock);
    }

    /*
    printf("OPT:\n");
    applyBlock->writeCode(stdout);
    printf("-----------------------------------\n");
    */
}

bool ASTApplyBlockOptimizer::SimplifyTableApplyIfs(AST *block, const std::set<std::string>& boolVariablesSetTrueOnce, const std::vector<OptimizerStatementSuccessors>& successorGraph, std::unordered_set<AST*> *toDelete){
    bool changed = false;
    
    if(block == NULL || block->children.size() == 0)
        return changed;
    
    if(block->nType != NT_STAT_OR_DEC_LIST)
        return changed;

    size_t sit = 0;
    while(sit < block->children.size()){
        AST *statement = block->children[sit];

        if(statement == NULL){
            sit++;
            continue;
        }

        if(toDelete->find(statement) != toDelete->end()){
            toDelete->erase(statement);
            delete statement;
            block->children.erase(block->children.begin() + sit);
            continue;
        }

        if(statement->nType == NT_CONDITIONAL_STATEMENT){
            ASTApplyBlockOptimizer::SimplifyTableApplyIfs(statement->children[1], boolVariablesSetTrueOnce, successorGraph, toDelete);
            ASTApplyBlockOptimizer::SimplifyTableApplyIfs(statement->children[2], boolVariablesSetTrueOnce, successorGraph, toDelete);
        }
        else if(statement->nType == NT_SWITCH_STATEMENT){
            AST *switchCaseList = statement->getChildNT(NT_SWITCH_CASE_LIST);
            for(AST *switchCase : switchCaseList->children){
                if(switchCase->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
                    ASTApplyBlockOptimizer::SimplifyTableApplyIfs(switchCase->getChildNT(NT_STAT_OR_DEC_LIST), boolVariablesSetTrueOnce, successorGraph, toDelete);    
                }
            }
        }
        else if(statement->nType == NT_ASSIGN_STATEMENT){            
            const bool onlyOnce = boolVariablesSetTrueOnce.find(statement->children[0]->value) != boolVariablesSetTrueOnce.end();
            if(onlyOnce){
                size_t idx = 0;
                while(idx < successorGraph.size() && successorGraph[idx].statement != statement)
                    idx++;
                Assert(idx < successorGraph.size());

                if(successorGraph[idx].nextStatements.size() == 1){
                    AST *nextStatement = successorGraph[idx].nextStatements[0];
                    if(nextStatement->nType == NT_CONDITIONAL_STATEMENT && nextStatement->children[0]->value == statement->children[0]->value){
                        Assert(nextStatement->children[1] != NULL);
                        Assert(nextStatement->children[1]->nType == NT_STAT_OR_DEC_LIST);
                        Assert(nextStatement->children[2] == NULL);

                        toDelete->insert(nextStatement);
                        
                        block->children.erase(block->children.begin() + sit);
                        for(size_t j = 0; j < nextStatement->children[1]->children.size(); j++){
                            block->children.insert(block->children.begin() + sit, nextStatement->children[1]->children[nextStatement->children[1]->children.size() - j - 1]);
                            sit--;
                        }
                        nextStatement->children[1]->children.clear();

                        delete statement;

                        changed = true;
                    }
                }
            }
        }

        sit++;
    }

    return changed;
}

bool ASTApplyBlockOptimizer::SimplifyAdjacentIfs(AST *block){
    bool changed = false;
    
    //TODO: Switch cases
    if(block == NULL || block->children.size() == 0)
        return changed;
    
    Assert(block->nType == NT_STAT_OR_DEC_LIST);

    size_t sit = 0;
    while(sit < block->children.size() - 1){
        AST *statement = block->children[sit];
        
        if(statement->nType == NT_CONDITIONAL_STATEMENT && statement->children[0]->children.size() == 0){
            ASTApplyBlockOptimizer::SimplifyAdjacentIfs(statement->children[1]);
            ASTApplyBlockOptimizer::SimplifyAdjacentIfs(statement->children[2]);

            if(ASTApplyBlockOptimizer::ASTCheckUnchangedVar(statement, statement->children[0]->value)){  
                AST *nextStatement = block->children[sit + 1];
                if(nextStatement->nType == NT_CONDITIONAL_STATEMENT && nextStatement->children[0]->value == statement->children[0]->value){
                    if(statement->children[1] != NULL){
                        if(nextStatement->children[1] != NULL){
                            Assert(statement->children[1]->nType == NT_STAT_OR_DEC_LIST); //If it is not, just need to write some code...
                            if(nextStatement->children[1]->nType == NT_STAT_OR_DEC_LIST){
                                for(AST *st : nextStatement->children[1]->children){
                                    statement->children[1]->addChild(st);
                                }
                                nextStatement->children[1]->children.clear();
                            }
                            else
                                statement->children[1]->addChild(nextStatement->children[1]);
                        }
                    }
                    else{
                        statement->children[1] = nextStatement->children[1];
                        nextStatement->children[1] = NULL;
                    }
                    if(statement->children[2] != NULL){
                        if(nextStatement->children[2] != NULL){
                            Assert(statement->children[2]->nType == NT_STAT_OR_DEC_LIST); //If it is not, just need to write some code...
                            if(nextStatement->children[2]->nType == NT_STAT_OR_DEC_LIST){
                                for(AST *st : nextStatement->children[2]->children){
                                    statement->children[2]->addChild(st);
                                }
                                nextStatement->children[2]->children.clear();
                            }
                            else
                                statement->children[2]->addChild(nextStatement->children[2]);
                        }
                    }
                    else{
                        statement->children[2] = nextStatement->children[2];
                        nextStatement->children[2] = NULL;
                    }

                    delete nextStatement;
                    block->children.erase(block->children.begin() + sit + 1);
                    sit--;
                    changed = true;
                }
                else if(nextStatement->nType == NT_CONDITIONAL_STATEMENT && nextStatement->children[0]->value == "!" && nextStatement->children[0]->children[0]->value == statement->children[0]->value){
                    if(statement->children[1] != NULL){
                        if(nextStatement->children[2] != NULL){
                            Assert(statement->children[1]->nType == NT_STAT_OR_DEC_LIST); //If it is not, just need to write some code...
                            if(nextStatement->children[2]->nType == NT_STAT_OR_DEC_LIST){
                                for(AST *st : nextStatement->children[2]->children){
                                    statement->children[1]->addChild(st);
                                }
                                nextStatement->children[2]->children.clear();
                            }
                            else
                                statement->children[1]->addChild(nextStatement->children[2]);
                        }
                    }
                    else{
                        statement->children[1] = nextStatement->children[2];
                        nextStatement->children[2] = NULL;
                    }
                    if(statement->children[2] != NULL){
                        if(nextStatement->children[1] != NULL){
                            Assert(statement->children[2]->nType == NT_STAT_OR_DEC_LIST); //If it is not, just need to write some code...
                            if(nextStatement->children[1]->nType == NT_STAT_OR_DEC_LIST){
                                for(AST *st : nextStatement->children[1]->children){
                                    statement->children[2]->addChild(st);
                                }
                                nextStatement->children[1]->children.clear();
                            }
                            else
                                statement->children[2]->addChild(nextStatement->children[1]);
                        }
                    }
                    else{
                        statement->children[2] = nextStatement->children[1];
                        nextStatement->children[1] = NULL;
                    }

                    delete nextStatement;
                    block->children.erase(block->children.begin() + sit + 1);
                    sit--;
                    changed = true;
                }
            }
        }

        sit++;
    }

    return changed;
}

bool ASTApplyBlockOptimizer::SimplifySimilarSwitchCases(AST *block){
    bool changed = false;
    
    size_t sit = 0;

    while(sit < block->children.size()){
        AST *statement = block->children[sit];

        if(statement == NULL){
            sit++;
            continue;
        }

        if(statement->nType == NT_SWITCH_STATEMENT && statement->children[0]->value == TEMP_PROGRAM_CODE){
            bool skip = false;

            AST *switchCaseList = statement->getChildNT(NT_SWITCH_CASE_LIST);
            Assert(switchCaseList != NULL);

            size_t minChilds = 999999;
            std::vector<AST*> casesSODLs;
            for(AST *switchCase : switchCaseList->children){
                AST *caseSODL = switchCase->getChildNT(NT_STAT_OR_DEC_LIST);
                if(caseSODL == NULL)
                    skip = true;
                else if(caseSODL->children.size() < minChilds)
                    minChilds = caseSODL->children.size();
                casesSODLs.push_back(caseSODL);
            }

            if(skip){
                sit++;
                continue;
            }

            size_t ridx = 0;
            bool done = ridx >= minChilds;
            while(!done){
                bool equal = true;
                AST *statementToCompare = casesSODLs[0]->children[casesSODLs[0]->children.size() - 1 - ridx];
                for(size_t i = 1; i < casesSODLs.size(); i++){
                    if(!AST::Equals(casesSODLs[i]->children[casesSODLs[i]->children.size() - 1 - ridx], statementToCompare)){
                        equal = false;
                    }
                }

                if(equal)
                    ridx++;
                
                done = ridx >= minChilds || !equal;
            }

            /*
            printf("SS: %lu %lu\n", ridx, minChilds);
            statement->writeCode(stdout);
            printf("Equals:\n");
            for(size_t i = 0; i < ridx; i++){
                Assert(casesSODLs[0]->children.size() > i);
                casesSODLs[0]->children[casesSODLs[0]->children.size() - 1 - i]->writeCode(stdout);
            }
            */

            for(size_t i = 0; i < ridx; i++){
                block->children.insert(block->children.begin() + sit + 1, casesSODLs[0]->children[casesSODLs[0]->children.size() - 1 - i]->Clone());
            }
            for(AST *switchCase : switchCaseList->children){
                for(size_t i = 0; i < ridx; i++){
                    delete switchCase->getChildNT(NT_STAT_OR_DEC_LIST)->children[switchCase->getChildNT(NT_STAT_OR_DEC_LIST)->children.size() - 1];
                    switchCase->getChildNT(NT_STAT_OR_DEC_LIST)->children.erase(switchCase->getChildNT(NT_STAT_OR_DEC_LIST)->children.end() - 1);
                }
            }

            /*
            printf("SS: %lu %lu\n", ridx, minChilds);
            statement->writeCode(stdout);
            printf("Equals:\n");
            for(size_t i = 0; i < ridx; i++){
                block->children[sit + i + 1]->writeCode(stdout);
            }
            */

            if(ridx > 0){
                changed = true;
            }
        }

        sit++;
    }

    return changed;
}

bool ASTApplyBlockOptimizer::SimplifySubsequentIfsFromVars(AST *block){
    //TODO: Switch Cases 
    bool changed = false;

    if(block == NULL || block->children.size() == 0)
        return changed;

    Assert(block->nType == NT_STAT_OR_DEC_LIST);

    size_t sit = 0;
    while(sit < block->children.size() - 1){
        AST *statement = block->children[sit];

        if(statement->nType == NT_CONDITIONAL_STATEMENT){
            if(statement->children[1] != NULL && statement->children[1]->nType == NT_STAT_OR_DEC_LIST && statement->children[1]->children.size() > 0){
                AST *firstStatement = statement->children[1]->children[0];
                if(firstStatement->nType == NT_ASSIGN_STATEMENT){
                    const std::string varName = firstStatement->children[0]->value;
                    if(firstStatement->children[1]->value == "true"){
                        AST *nextStatement = block->children[sit + 1];
                        if(nextStatement->nType == NT_CONDITIONAL_STATEMENT && nextStatement->children[0]->value == varName){
                            if(nextStatement->children[1] != NULL){
                                if(nextStatement->children[1]->nType == NT_STAT_OR_DEC_LIST){
                                    for(AST *st : nextStatement->children[1]->children){
                                        statement->children[1]->addChild(st);
                                    }
                                    nextStatement->children[1]->children.clear();
                                }
                                else
                                    statement->children[1]->addChild(nextStatement->children[1]);
                            }
                            if(nextStatement->children[2] != NULL){
                                if(statement->children[2] != NULL){
                                    if(nextStatement->children[2]->nType == NT_STAT_OR_DEC_LIST){
                                        for(AST *st : nextStatement->children[2]->children){
                                            statement->children[2]->addChild(st);
                                        }
                                        nextStatement->children[2]->children.clear();
                                    }
                                    else
                                        statement->children[2]->addChild(nextStatement->children[2]);
                                }
                                else{
                                    statement->children[2] = nextStatement->children[2];
                                    nextStatement->children[2] = NULL;
                                }
                            }

                            delete nextStatement;
                            block->children.erase(block->children.begin() + sit + 1);
                            sit--;
                            changed = true;
                        }
                    }
                }
            }

            ASTApplyBlockOptimizer::SimplifySubsequentIfsFromVars(statement->children[1]);
            ASTApplyBlockOptimizer::SimplifySubsequentIfsFromVars(statement->children[2]);
        }

        sit++;
    }

    return changed;
}

void ASTApplyBlockOptimizer::OptimizeBlock(AST *block, std::unordered_map<std::string, OptimizerBoolVariableValue>* boolVariables){
    Assert(block->nType == NT_STAT_OR_DEC_LIST);

    size_t sit = 0;
    while(sit < block->children.size()){
        AST *statement = block->children[sit];

        if(statement == NULL){
            sit++;
            continue;
        }

        switch(statement->nType){
            case NT_VARIABLE_DECLARATION:
            {
                OptimizerBoolVariableValue value;
                if(statement->children[2] == NULL)
                    value = OPTIMIZER_BOOL_VARIABLE_UNKNOWN;
                else if(statement->children[2]->value == "true")
                    value = OPTIMIZER_BOOL_VARIABLE_TRUE;
                else
                    value = OPTIMIZER_BOOL_VARIABLE_FALSE;
                
                boolVariables->emplace(statement->value, value);

                //printf("Declared %s as %s\n", statement->value.c_str(), ASTApplyBlockOptimizer::OBVVToString(value).c_str());
            }
                break;
            case NT_STAT_OR_DEC_LIST:
                ASTApplyBlockOptimizer::OptimizeBlock(statement, boolVariables);
                break;
            case NT_ASSIGN_STATEMENT:
            {
                auto it = boolVariables->find(statement->children[0]->value);
                if(it != boolVariables->end()){
                    OptimizerBoolVariableValue value;
                    if(statement->children[1]->value == "true")
                        value = OPTIMIZER_BOOL_VARIABLE_TRUE;
                    else if(statement->children[1]->value == "false")
                        value = OPTIMIZER_BOOL_VARIABLE_FALSE;
                    else
                        value = OPTIMIZER_BOOL_VARIABLE_UNKNOWN;
                    
                    it->second = value;

                    //printf("Assigned %s as %s\n", statement->children[0]->value.c_str(), ASTApplyBlockOptimizer::OBVVToString(value).c_str());
                }
            }
                break;
            case NT_CONDITIONAL_STATEMENT:
            {
                auto it = boolVariables->find(statement->children[0]->value);
                if(it != boolVariables->end()){
                    
                    if(it->second == OPTIMIZER_BOOL_VARIABLE_UNKNOWN) {
                        std::unordered_map<std::string, OptimizerBoolVariableValue> varsCaseTrue = *boolVariables;
                        std::unordered_map<std::string, OptimizerBoolVariableValue> varsCaseFalse = *boolVariables;
                    
                        varsCaseTrue[statement->children[0]->value] = OPTIMIZER_BOOL_VARIABLE_TRUE;
                        varsCaseFalse[statement->children[0]->value] = OPTIMIZER_BOOL_VARIABLE_FALSE;

                        //printf("Begin Conditional True\n");
                        if(statement->children[1] != NULL){
                            AST *tempBlock = new AST("", NT_STAT_OR_DEC_LIST);
                            tempBlock->addChild(statement->children[1]);
                            ASTApplyBlockOptimizer::OptimizeBlock(tempBlock, &varsCaseTrue);
                            tempBlock->children.clear();
                            delete tempBlock;
                        }

                        //printf("Begin Conditional False\n");
                        if(statement->children[2] != NULL){
                            AST *tempBlock = new AST("", NT_STAT_OR_DEC_LIST);
                            tempBlock->addChild(statement->children[2]);
                            ASTApplyBlockOptimizer::OptimizeBlock(tempBlock, &varsCaseFalse);
                            tempBlock->children.clear();
                            delete tempBlock;
                        }
                    
                        for(auto it = boolVariables->begin(); it != boolVariables->end(); it++){
                            if(varsCaseTrue[it->first] != varsCaseFalse[it->first]){
                                it->second = OPTIMIZER_BOOL_VARIABLE_UNKNOWN;
                                //printf("Changed %s to unknown\n", it->first.c_str());
                            }
                        }

                        //printf("End Conditional\n");
                    }
                    else if(it->second == OPTIMIZER_BOOL_VARIABLE_TRUE){
                        if(statement->children[1] != NULL){
                            AST *tempBlock = new AST("", NT_STAT_OR_DEC_LIST);
                            tempBlock->addChild(statement->children[1]);
                            ASTApplyBlockOptimizer::OptimizeBlock(tempBlock, boolVariables);
                            tempBlock->children.clear();
                            delete tempBlock;

                            block->children[sit] = statement->children[1];
                            statement->children.erase(statement->children.begin() + 1);
                            delete statement;
                        }
                        else{
                            delete block->children[sit];
                            block->children.erase(block->children.begin() + sit);
                            sit--;
                        }
                    }
                    else if(it->second == OPTIMIZER_BOOL_VARIABLE_FALSE){
                        if(statement->children[2] != NULL){
                            AST *tempBlock = new AST("", NT_STAT_OR_DEC_LIST);
                            tempBlock->addChild(statement->children[2]);
                            ASTApplyBlockOptimizer::OptimizeBlock(tempBlock, boolVariables);
                            tempBlock->children.clear();
                            delete tempBlock;

                            block->children[sit] = statement->children[2];
                            statement->children.erase(statement->children.begin() + 2);
                            delete statement;
                        }
                        else{
                            delete block->children[sit];
                            block->children.erase(block->children.begin() + sit);
                            sit--;
                        }
                    }
                }
                else{
                    std::unordered_map<std::string, OptimizerBoolVariableValue> varsCaseTrue = *boolVariables;
                    std::unordered_map<std::string, OptimizerBoolVariableValue> varsCaseFalse = *boolVariables;
                
                    //printf("Begin Conditional True\n");
                    if(statement->children[1] != NULL){
                        AST *tempBlock = new AST("", NT_STAT_OR_DEC_LIST);
                        tempBlock->addChild(statement->children[1]);
                        ASTApplyBlockOptimizer::OptimizeBlock(tempBlock, &varsCaseTrue);
                        tempBlock->children.clear();
                        delete tempBlock;
                    }

                    //printf("Begin Conditional False\n");
                    if(statement->children[2] != NULL){
                        AST *tempBlock = new AST("", NT_STAT_OR_DEC_LIST);
                        tempBlock->addChild(statement->children[2]);
                        ASTApplyBlockOptimizer::OptimizeBlock(tempBlock, &varsCaseFalse);
                        tempBlock->children.clear();
                        delete tempBlock;
                    }

                    for(auto it = boolVariables->begin(); it != boolVariables->end(); it++){
                        if(varsCaseTrue[it->first] != varsCaseFalse[it->first]){
                            it->second = OPTIMIZER_BOOL_VARIABLE_UNKNOWN;
                            //printf("Changed %s to unknown\n", it->first.c_str());
                        }
                    }

                    //printf("End Conditional\n");
                }
            }
                break;
            case NT_SWITCH_STATEMENT:
                if(statement->getChildNT(NT_SWITCH_CASE_LIST)->children.size() > 0){
                    //printf("Begin Switch Case\n");
                    std::vector<std::unordered_map<std::string, OptimizerBoolVariableValue>> boolVariablePermutations;
                    for(AST *switchCase : statement->getChildNT(NT_SWITCH_CASE_LIST)->children){
                        //printf("Begin Case\n");
                        if(switchCase->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
                            std::unordered_map<std::string, OptimizerBoolVariableValue> newPM = *boolVariables;
                            ASTApplyBlockOptimizer::OptimizeBlock(switchCase->getChildNT(NT_STAT_OR_DEC_LIST), &newPM);
                            boolVariablePermutations.push_back(newPM);
                        }
                        //printf("End Case\n");
                    }
                    //printf("Conflicting Vars:\n");
                    if(boolVariablePermutations.size() > 0){
                        *boolVariables = boolVariablePermutations[0];
                        for(size_t i = 1; i < boolVariablePermutations.size(); i++){
                            for(auto it = boolVariables->begin(); it != boolVariables->end(); it++){
                                if(it->second != boolVariablePermutations[i][it->first]){
                                    it->second = OPTIMIZER_BOOL_VARIABLE_UNKNOWN;
                                    //printf("Changed %s to unknown\n", it->first.c_str());
                                }
                            }
                        }
                    }
                    //printf("End Switch Case\n");
                }
                break;
            default:
                break;
        }
        sit++;
    }
}

void ASTApplyBlockOptimizer::RemoveUnusedVariables(AST *ast){ 
    std::vector<std::string> unusedVars;

    for(AST *c : ast->children)
        if(c != NULL && c->nType == NT_VARIABLE_DECLARATION && ASTApplyBlockOptimizer::ASTCheckUnusedVar(ast, c->value))
            unusedVars.push_back(c->value);

    for(const std::string& varName : unusedVars)
        ASTApplyBlockOptimizer::ASTRemoveUnusedVar(ast, varName);
}

bool ASTApplyBlockOptimizer::ASTCheckUnusedVar(AST *ast, const std::string& varName){
    if(ast == NULL)
        return true;

    for(size_t i = ((ast->getNodeType() == NT_ASSIGN_STATEMENT) ? 1 : 0); i < ast->getChildren().size(); i++)
        if(!ASTApplyBlockOptimizer::ASTCheckUnusedVar(ast->children[i], varName))
            return false;

    if(ast->nType != NT_VARIABLE_DECLARATION && ast->value == varName)
        return false;

    return true;
}

bool ASTApplyBlockOptimizer::ASTCheckUnchangedVar(AST *ast, const std::string& varName){
    if(ast == NULL)
        return true;

    for(size_t i = 0; i < ast->getChildren().size(); i++)
        if(!ASTApplyBlockOptimizer::ASTCheckUnchangedVar(ast->children[i], varName))
            return false;

    if(ast->nType == NT_ASSIGN_STATEMENT && ast->children[0]->value == varName)
        return false;

    return true;
}

void ASTApplyBlockOptimizer::ASTRemoveUnusedVar(AST *ast, const std::string& varName){
    size_t i = 0;
    while(i < ast->children.size()){
        AST *c = ast->children[i];
        
        if((c != NULL && c->getNodeType() == NT_VARIABLE_DECLARATION && c->getValue() == varName) || (c != NULL && c->getNodeType() == NT_ASSIGN_STATEMENT && c->children[0]->getValue() == varName)){
            ast->RemoveChild(c);
            delete c;
            i--;
        }

        i++;
    }

    for(AST *c : ast->getChildren())
        if(c != NULL)
            ASTApplyBlockOptimizer::ASTRemoveUnusedVar(c, varName);
}

void ASTApplyBlockOptimizer::GetVariablesSetTrueOnceRec(AST *block, std::unordered_map<std::string, int>* boolVariables){
    for(AST *statement : block->children){

        if(statement == NULL)
            continue;

        switch(statement->nType){
            case NT_VARIABLE_DECLARATION:
                boolVariables->emplace(statement->value, 0);
                break;
            case NT_ASSIGN_STATEMENT:
                if(statement->children[1]->value == "true"){
                    auto it = boolVariables->find(statement->children[0]->value);
                    if(it != boolVariables->end())
                        it->second++;
                }
                break;
            default:
                for(AST *c : statement->children)
                    if(c != NULL)
                        ASTApplyBlockOptimizer::GetVariablesSetTrueOnceRec(c, boolVariables);
                break;
        }
    }
}

std::set<std::string> ASTApplyBlockOptimizer::GetVariablesSetTrueOnce(AST *block){
    std::set<std::string> boolVariables;

    if(block == NULL)
        return boolVariables;

    std::unordered_map<std::string, int> boolVariablesMap;
    ASTApplyBlockOptimizer::GetVariablesSetTrueOnceRec(block, &boolVariablesMap);

    for(auto it : boolVariablesMap)
        if(it.second == 1)
            boolVariables.emplace(it.first);

    return boolVariables;
}

void ASTApplyBlockOptimizer::GetSucessorGraphRec(AST *block, std::vector<OptimizerStatementSuccessors> *successors, std::vector<size_t>* hangingIDXs){
    size_t sit = 0;
    while(sit < block->children.size()){
        AST *statement = block->children[sit];

        if(statement == NULL){
            sit++;
            continue;
        }

        for(size_t h : *hangingIDXs){
            successors->at(h).nextStatements.push_back(statement);
        }
        hangingIDXs->clear();

        bool hang = true;
        successors->push_back({statement, {}});
        const size_t ossIDX = successors->size() - 1;

        if(statement->nType == NT_CONDITIONAL_STATEMENT){
            std::vector<size_t> hangingTrue;
            std::vector<size_t> hangingFalse;
            if(statement->children[1] != NULL){
                if(statement->children[1]->nType == NT_STAT_OR_DEC_LIST){
                    Assert(statement->children[1]->children.size() > 0);
                    successors->at(ossIDX).nextStatements.push_back(statement->children[1]->children[0]);
                    ASTApplyBlockOptimizer::GetSucessorGraphRec(statement->children[1], successors, &hangingTrue);
                }
                else{
                    successors->at(ossIDX).nextStatements.push_back(statement->children[1]);
                    successors->push_back({statement->children[1], {}});
                    hangingIDXs->push_back(successors->size() - 1);
                }
            }
            if(statement->children[2] != NULL){
                if(statement->children[2]->nType == NT_STAT_OR_DEC_LIST){
                    Assert(statement->children[2]->children.size() > 0);
                    successors->at(ossIDX).nextStatements.push_back(statement->children[2]->children[0]);
                    ASTApplyBlockOptimizer::GetSucessorGraphRec(statement->children[2], successors, &hangingFalse);
                }
                else{
                    successors->at(ossIDX).nextStatements.push_back(statement->children[2]);
                    successors->push_back({statement->children[2], {}});
                    hangingIDXs->push_back(successors->size() - 1);
                }
            }

            for(size_t h : hangingTrue)
                hangingIDXs->push_back(h);
            for(size_t h : hangingFalse)
                hangingIDXs->push_back(h);

            if(successors->at(ossIDX).nextStatements.size() == 2)
                hang = false;
        }
        else if(statement->nType == NT_SWITCH_STATEMENT){
            AST *switchCaseList = statement->getChildNT(NT_SWITCH_CASE_LIST);
            for(AST *switchCase : switchCaseList->children){
                if(switchCase->getChildNT(NT_STAT_OR_DEC_LIST) != NULL){
                    Assert(switchCase->getChildNT(NT_STAT_OR_DEC_LIST)->children.size() > 0);
                    successors->at(ossIDX).nextStatements.push_back(switchCase->getChildNT(NT_STAT_OR_DEC_LIST)->children[0]);
                    ASTApplyBlockOptimizer::GetSucessorGraphRec(switchCase->getChildNT(NT_STAT_OR_DEC_LIST), successors, hangingIDXs);
                }
            }
            hang = true;
        }
        else{
            if(sit < block->children.size() - 1){
                successors->at(ossIDX).nextStatements.push_back(block->children[sit + 1]);
                hang = false;
            }
        }

        if(hang)
            hangingIDXs->push_back(ossIDX);
        
        sit++;
    }
}

std::vector<OptimizerStatementSuccessors> ASTApplyBlockOptimizer::GetSucessorGraph(AST *block){
    std::vector<size_t> hanging;
    std::vector<OptimizerStatementSuccessors> successors;
    ASTApplyBlockOptimizer::GetSucessorGraphRec(block, &successors, &hanging);
    return successors;
}

void ASTApplyBlockOptimizer::GenStatementSuccessorsDot(const std::vector<OptimizerStatementSuccessors>& successors, FILE *fp){
    std::unordered_map<AST*, size_t> statementToIndex;
    fprintf(fp, "digraph G {\n");
    for(size_t i = 0; i < successors.size(); i++){
        statementToIndex.emplace(successors[i].statement, i);
        const OptimizerStatementSuccessors& oss = successors[i];
        fprintf(fp, "  NODE_%lu [label=\"%p\"];\n", i, oss.statement);
        
    }
    for(size_t i = 0; i < successors.size(); i++){
        const OptimizerStatementSuccessors& oss = successors[i];
        const size_t mIdx = statementToIndex.find(oss.statement)->second;
        for(AST *s : oss.nextStatements){
            fprintf(fp, "  NODE_%lu -> NODE_%lu;\n", mIdx, statementToIndex.find(s)->second);
        }
    }
    fprintf(fp, "}\n");
}

void ASTApplyBlockOptimizer::GenStatementSuccessorsDot(const std::vector<OptimizerStatementSuccessors>& successors, const std::string& fileName){
    FILE *fp = fopen(fileName.c_str(), "w");
    ASTApplyBlockOptimizer::GenStatementSuccessorsDot(successors, fp);
    fclose(fp);
}