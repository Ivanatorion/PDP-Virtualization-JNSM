#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/ASTSimplifier.h"

void ASTSimplifier::RemoveVirtPreventMergeAnnotations(AST *ast){
    for(AST *c : ast->children){
        if(c != NULL)
            ASTSimplifier::RemoveVirtPreventMergeAnnotations(c);
    }
    
    size_t i = 0;
    while(i < ast->children.size()){
        if(ast->children[i] != NULL && ast->children[i]->nType == NT_ANNOTATION_LIST){
            size_t j = 0;
            while(j < ast->children[i]->children.size()){
                Assert(ast->children[i]->children[j] != NULL);
                if(ast->children[i]->children[j]->nType == NT_ANNOTATION && ast->children[i]->children[j]->value == VIRT_PREVENT_MERGE_ANNOTATION){
                    delete ast->children[i]->children[j];
                    ast->children[i]->children.erase(ast->children[i]->children.begin() + j);
                    j--;
                }
                j++;
            }
            if(ast->children[i]->children.size() == 0){
                delete ast->children[i];
                ast->children[i] = NULL;
            }
        }
        i++;
    }
}

void ASTSimplifier::CheckUnusedHeaders(AST *ast){
    //TODO

    for(AST *c : ast->children)
        if(c != NULL)
            ASTSimplifier::CheckUnusedHeaders(c);
}