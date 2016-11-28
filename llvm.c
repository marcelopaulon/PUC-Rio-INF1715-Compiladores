#include "llvm.h"
#include "ast.h"

void genIdent(int level, FILE *fp);
void genLine(char * type, int line, int newLineAtEnd, FILE *fp);
void genType(Type *type, FILE *fp);
void genBinExpType(ExpE type, int nIdent, FILE *fp);
int genExp(Exp * exp, int nIdent, FILE *fp);
void genVar(Var * var, int nIdent, FILE *fp);
void genCmdCall (CmdCall * cmd, int nIdent, FILE *fp);
void genCmdBasic(CmdBasic * cmd, int nIdent, FILE *fp);
void genExpList(ExpList * list, int nIdent, FILE *fp);
void genDefVarList(DefVarList * list, int nIdent, FILE *fp, int isGlobal);
void genCmdList(CmdList * list, int nIdent, FILE *fp);
void genParamList(ParamList * paramlist, int nIdent, FILE *fp);
void genBlock(Block * block, int nIdent, FILE *fp);
void genCmd(Cmd * cmd, int nIdent, FILE *fp);
void genParam(Param * param, int nIdent, FILE *fp);
void genDefFunc(Func * deffunc, int nIdent, FILE *fp);
void genDefVar(DefVar * defvar, int nIdent, FILE *fp, int isGlobal);
void genDefinition(Definition *definition, int nIdent, FILE *fp);

static int curTempVar = 0;

int getNextTempVar()
{
    return ++curTempVar;
}

void createLLVM(DefinitionList *tree, FILE *fp)
{
    if(tree == NULL)
    {
        printf("FCLOSED");
        fclose(fp);
        return;
    }

    genDefinition(tree->definition, 0, fp);
    
    createLLVM(tree->next, fp);
}

void genDefinition(Definition *definition, int nIdent, FILE *fp)
{
    fprintf(fp, ";TODO: Gen definition\n");

    if(definition->type == TypeDefVar)
    {
        genDefVarList(definition->u.defvarlist, nIdent+1, fp, 1);
    }
    else if(definition->type == TypeDefFunc)
    {
        genDefFunc(definition->u.deffunc, nIdent+1, fp);
    }
    else
    {
        printf("Error - Invalid definition type. Exiting.\n");
        fclose(fp);
        exit(-1);
    }
}

void genDefVar(DefVar * defvar, int nIdent, FILE * fp, int isGlobal)
{
    genIdent(nIdent, fp);

    defvar->isGlobal = isGlobal;

    defvar->varNumber = getNextTempVar();

    if(isGlobal != 0) {
        fprintf(fp, "@t%d = common global ", defvar->varNumber);
        genType(defvar->type, fp);
        fprintf(fp, " 0\n");
    }
    else {
        fprintf(fp, "%%t%d = alloca ", defvar->varNumber);
        genType(defvar->type, fp);
        fprintf(fp, "\n");
    }
}

void genDefFunc(Func * deffunc, int nIdent, FILE * fp)
{
    genIdent(nIdent, fp);
    fprintf(fp, "define ");

    genType(deffunc->type, fp);

    //deffunc->varNumber = getNextTempVar();
    fprintf(fp, " @%s(", deffunc->id);

    genParamList(deffunc->params, nIdent + 1, fp);

    fprintf(fp, ") #0 ");

    fprintf(fp, "{\n");
    genBlock(deffunc->block, nIdent, fp);
    fprintf(fp, "}\n");

}

void genParam(Param * param, int nIdent, FILE * fp)
{
    genIdent(nIdent, fp);
    fprintf(fp, "Parameter: %s\n", param->id);

    genType(param->type, fp);
}

void genIdent(int level, FILE *fp){
    int i;
    for(i = 0; i < level; i++) {
        fprintf(fp, "  ");
    }
}

void genLine(char * type, int line, int newLineAtEnd, FILE *fp){
}

void genType(Type *type, FILE *fp) {
    switch(type->name){
        case VarFloat:
            fprintf(fp,"float");
            break;
        case VarInt:
            fprintf(fp,"i32");
            break;
        case VarChar:
            break;
    }
}

void genParamList(ParamList * paramlist, int nIdent, FILE *fp){
}

void genCmdList(CmdList *list, int nIdent, FILE *fp)
{
    CmdList * current;

    if(list == NULL)
    {
        return;
    }

    for(current = list; current != NULL; current = current->next)
    {
        genCmd(current->cmd, nIdent, fp);
    }
}

void genBlock(Block * block, int nIdent, FILE *fp){
    genDefVarList(block->vars, nIdent+1, fp, 0);
    genCmdList(block->cmds, nIdent+1, fp);

    genIdent(nIdent, fp);
}

void genDefVarList(DefVarList * list, int nIdent, FILE *fp, int isGlobal)
{
    DefVarList * current;
    
    if(list == NULL)
    {
        return;
    }

    for(current = list; current != NULL; current = current->next)
    {
        genDefVar(current->defvar, nIdent+1, fp, isGlobal);
    }
}

void genCmd(Cmd *cmd, int nIdent, FILE *fp) {
    genExp(cmd->e, nIdent, fp);

    switch(cmd->type){
        case CmdWhile:
            //genCmd(cmd->u.cmd, nIdent);
            break;
        case CmdIf:
            //printf("If\n");
            //genCmd(cmd->u.cmd, nIdent);
            break;
        case CmdIfElse:
            //printf("If and Else\n");
            //genCmd(cmd->u.cmds.c1, nIdent);
            //genCmd(cmd->u.cmds.c2, nIdent);
            break;
        case CmdBasicE:
            genCmdBasic(cmd->u.cmdBasic, nIdent, fp);
            break;
        default:
            printf("Invalid command type. Exiting.\n");
            exit(-2);
    }
}

void genBinExpType(ExpE type, int nIdent, FILE *fp) {

}

union fodase
{
    double d;
    long long l;
};

void doubleToHex(double d, char *strOut){
    sprintf(strOut, "%e", (float)d);
}

int genExp(Exp *exp, int nIdent, FILE *fp) {
    if(exp == NULL)
    {
        return -3;
    }

    switch(exp->tag){
        case ExpAdd:
        case ExpSub:
        case ExpMul:
        case ExpDiv: {
            int e1 = genExp(exp->u.bin.e1, nIdent, fp);
            int e2 = genExp(exp->u.bin.e2, nIdent, fp);
            int ret = getNextTempVar();
            char *op;
            char *type;

            if(exp->u.bin.e1->type->name == VarFloat)
            {
                type = "float";

                if(exp->tag == ExpAdd) op = "fadd";
                else if(exp->tag == ExpSub) op = "fsub";
                else if(exp->tag == ExpMul) op = "fmul";
                else  op = "fdiv";
            }
            else
            {
                type = "i32";

                if(exp->tag == ExpAdd) op = "add";
                else if(exp->tag == ExpSub) op = "sub";
                else if(exp->tag == ExpMul) op = "mul";
                else  op = "sdiv";
            }

            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = %s %s %%t%d, %%t%d\n", ret, op, type, e1, e2);
            return ret;
        }
        case ExpEqual:
        case ExpLess:
        case ExpGreater:
        case ExpLessEqual:
        case ExpGreaterEqual:
        case ExpOr:
        case ExpAnd:
            //printBinExpType(exp->tag, nIdent);
            //printIdent(nIdent);
            //printf("Expression resulting ");
            //printType(exp->type, 0);
            //printExp(exp->u.bin.e1, nIdent);
            //printExp(exp->u.bin.e2, nIdent);
            break;
        case ExpVar: {
            //exp->u.var
            //char *storeId;

//            switch(exp->u.var->tag){
//                case VarId:
//                    if(exp->u.var->u.def.dec->id != NULL)
//                    {
//                        storeId = exp->u.var->u.def.dec->id;
//                    }
//                    else
//                    {
//                        printf("Null variable identifier. Exiting.\n");
//                        exit(-1);
//                    }
//                    break;
//                case VarIndexed:
//                    //printf("Indexed\n");
//                    //printExp(var->u.indexed.e1, nIdent + 1);
//                    //printExp(var->u.indexed.e2, nIdent + 1);
//                    break;
//                default:
//                    printf("Invalid variable tag type. Exiting.\n");
//                    exit(-4);
//            }
// EXP VAR(Var) =  EXP(a + b);
            // Var
            //store i32 2, i32* %b

//            int temp = getNextTempVar(), ret = getNextTempVar();
//            genIdent(nIdent, fp);
//            fprintf(fp, "%%t%d = alloca i32\n", temp);
//            genIdent(nIdent, fp);
//            fprintf(fp, "store i32 %d, i32* %%t%d\n", exp->u.l, temp);
//            genIdent(nIdent, fp);
//            fprintf(fp, "%%t%d = load i32* %%t%d\n", ret, temp);
            int ret = getNextTempVar();
            char *type;

            if(exp->u.var->u.def.dec->type->name == VarFloat)
            {
                type = "float";
            }
            else
            {
                type = "i32";
            }

            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = load %s* %%t%d\n", ret, type, exp->u.var->u.def.dec->varNumber);
            return ret;
        }

        case ExpCall:
            //printIdent(nIdent);
            //printf("Expression type: Function Call\n");
            //printIdent(nIdent);
            //printf("Expression resulting ");
            //printType(exp->type, 0);
            //printCmdCall(exp->u.call, nIdent+1);
            break;
        case ExpNot:
            //printIdent(nIdent);
            //printf("Expression type: Negation\n");
            //printIdent(nIdent);
            //printf("Expression resulting ");
            //printType(exp->type, 0);
            //printExp(exp->u.un, nIdent);
            break;
        case ExpMinus:
            //printIdent(nIdent);
            //printf("Expression type: Negative\n");
            //printIdent(nIdent);
            //printf("Expression resulting ");
            //printType(exp->type, 0);
            //printExp(exp->u.un, nIdent);
            break;
        case ExpNew:
//            printIdent(nIdent);
//            printf("Expression type: New\n");
//            printType(exp->u.newexp.type, nIdent);
//            printIdent(nIdent);
//            printf("Expression resulting ");
//            printType(exp->type, 0);
//            printExp(exp->u.newexp.exp, nIdent);
            break;
        case ExpString:
//            printIdent(nIdent);
//            printf("Expression type: String\n");
//            printIdent(nIdent);
//            printf("Expression resulting ");
//            printType(exp->type, 0);
//            printIdent(nIdent);
//            printf("%s\n", exp->u.c);
            break;
        case ExpInt: {
            int temp = getNextTempVar();
            int ret = getNextTempVar();
            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = alloca i32\n", temp);
            genIdent(nIdent, fp);
            fprintf(fp, "store i32 %d, i32* %%t%d\n", exp->u.l, temp);
            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = load i32* %%t%d\n", ret, temp);
            return ret;
        }
        case ExpFloat: {
            int temp = getNextTempVar();
            int ret = getNextTempVar();
            char floatValHex[19];
            doubleToHex(exp->u.d, floatValHex);

            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = alloca float\n", temp);
            genIdent(nIdent, fp);
            fprintf(fp, "store float %s, float* %%t%d\n", floatValHex, temp);
            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = load float* %%t%d\n", ret, temp);
            return ret;
        }
        case ExpCastIntToFloat: {
            int temp = genExp(exp->u.un, nIdent + 1, fp);
            int ret = getNextTempVar();

            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = sitofp i32 %%t%d to float\n", ret, temp);

            return ret;
        }
        default:
            printf("Invalid expression type %d. Exiting\n", exp->tag);
            exit(-5);
    }

    return -2;
}

void genVar(Var *var, int nIdent, FILE *fp) {
    //printIdent(nIdent);
    //printf("Expression type: Variable\n");
    //printIdent(nIdent);
    //printf("Expression resulting ");
    //printType(exp->type, 0);
    //printVar(exp->u.var, nIdent+1);


    // TODO: Tratar varindexed

    if(!var->u.def.dec->isGlobal) {
        fprintf(fp, "%%t%d", var->u.def.dec->varNumber);
    }
    else
    {
        fprintf(fp, "@t%d", var->u.def.dec->varNumber);
    }
}

void genCmdCall(CmdCall *cmd, int nIdent, FILE *fp) {

}

void genExpList(ExpList *list, int nIdent, FILE *fp) {

}

void genCmdBasic(CmdBasic *cmd, int nIdent, FILE *fp) {
    switch(cmd->type){
        case CmdBasicReturn: {
            int temp = genExp(cmd->u.returnExp, nIdent, fp);
            genIdent(nIdent, fp);
            fprintf(fp, "ret ");
            genType(cmd->u.returnExp->type, fp);
            fprintf(fp, " %%t%d\n", temp);
            break;
        }
        case CmdBasicCall:
            //printf("Function call\n");
            //genCmdCall(cmd->u.call, nIdent, fp);
            break;
        case CmdBasicBlock:
            genBlock(cmd->u.block, nIdent, fp);
            break;
        case CmdBasicVar: {
            int expNameId = genExp(cmd->u.varCmd.exp, nIdent, fp);

            // TODO CHeck e = -1, then get var name
            genIdent(nIdent, fp);
            fprintf(fp, "store ");
            genType(cmd->u.varCmd.exp->type, fp);
            fprintf(fp, " %%t%d, ", expNameId);

            switch(cmd->u.varCmd.var->tag)
            {
                case VarId:
                    genType(cmd->u.varCmd.var->u.def.dec->type, fp);
                    break;
                case VarIndexed:
                    // TODO
                    break;
            }

            fprintf(fp, "* ");
            genVar(cmd->u.varCmd.var, nIdent, fp);
            fprintf(fp, "\n");

            break;
        }
        default:
            printf("Invalid command type. Exiting.\n");
            exit(-3);
    }
}

