#include <string.h>
#include <math.h>
#include <stdio.h>

#include "llvm.h"
#include "ast.h"

void genIdent(int level, FILE *fp);
void genType(Type *type, FILE *fp);
void genWhile(Cmd *cmd, int nIdent, FILE *fp);
int genCond(Exp *e, int lt, int lf, int nIdent, FILE *fp);
int genExp(Exp * exp, int nIdent, FILE *fp);
void genVar(Var * var, int nIdent, FILE *fp);
int genCmdCall (CmdCall * cmd, int nIdent, FILE *fp);
void genCmdBasic(CmdBasic * cmd, int nIdent, FILE *fp);
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
static int curTempLabel = 0;

int getNextTempVar()
{
    return ++curTempVar;
}

int getNextTempLabel()
{
    return ++curTempLabel;
}

void genLabel(int label, FILE* fp){
    fprintf(fp, "\nbr label %%l%d\n", label);
    fprintf(fp, "\nl%d:\n", label);
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
        char *initValue;
        fprintf(fp, "@t%d = common global ", defvar->varNumber);
        genType(defvar->type, fp);

        if(defvar->type->name == VarFloat)
        {
            initValue = " 0.0";
        }
        else
        {
            initValue = " 0";
        }

        fprintf(fp, " %s\n", initValue);
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

    fprintf(fp, " @%s", deffunc->id);

    genParamList(deffunc->params, nIdent + 1, fp);

    fprintf(fp, " #0 ");

    fprintf(fp, "{\n");

    genBlock(deffunc->block, nIdent, fp);
    fprintf(fp, "}\n");

}

void genParam(Param * param, int nIdent, FILE * fp)
{
    genType(param->type, fp);
    param->varNumber = getNextTempVar();
    fprintf(fp, " %%t%d", param->varNumber);
}

void genIdent(int level, FILE *fp){
    int i;
    for(i = 0; i < level; i++) {
        fprintf(fp, "  ");
    }
}

char *getType(Type *type)
{
    switch(type->name){
        case VarFloat:
            return "float";
        case VarInt:
            return "i32";
        case VarChar:
            return "i8";
    }

    return "Invalid type";
}

void genType(Type *type, FILE *fp) {
    fprintf(fp, "%s", getType(type));
}

void genParamList(ParamList * paramlist, int nIdent, FILE *fp){
    ParamList * current;
    fprintf(fp,"(");
    for(current = paramlist; current != NULL; current = current->next) {
        genParam(current->param, nIdent, fp);
        if(current->next != NULL) fprintf(fp, ",");
    }
    fprintf(fp,")");
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
            genWhile(cmd, nIdent, fp);
            break;
        case CmdIf: {
            int lf = getNextTempLabel();
            int lt = getNextTempLabel();

            genCond(cmd->e, lt, lf, nIdent, fp);

            genLabel(lt, fp);
            genCmd(cmd->u.cmd, nIdent, fp);
            genLabel(lf, fp);
            break;
        }
        case CmdIfElse: {
            int lt = getNextTempLabel();
            int lf = getNextTempLabel();
            int lAfterElse = getNextTempLabel();

            genCond(cmd->e, lt, lf, nIdent, fp);

            genLabel(lt, fp);
            genCmd(cmd->u.cmds.c1, nIdent, fp);

            genIdent(nIdent, fp);
            fprintf(fp, "br label %%l%d\n", lAfterElse);

            genLabel(lf, fp);
            genCmd(cmd->u.cmds.c2, nIdent, fp);

            genLabel(lAfterElse, fp);

            break;
        }
        case CmdBasicE:
            genCmdBasic(cmd->u.cmdBasic, nIdent, fp);
            break;
        default:
            printf("Invalid command type. Exiting.\n");
            exit(-2);
    }
}

void genWhile(Cmd *cmd, int nIdent, FILE *fp){
    int lWhile = getNextTempLabel();
    int lBreak = getNextTempLabel();

    genCond(cmd->e, lWhile, lBreak, nIdent, fp);

    genLabel(lWhile, fp);
    genCmd(cmd->u.cmd, nIdent+1, fp);
    genCond(cmd->e, lWhile, lBreak, nIdent, fp);

    genLabel(lBreak, fp);
}

int genCondComp(Exp *e, char* opcode, int lt, int lf, int nIdent, FILE *fp) {
    int r1 = genExp(e->u.bin.e1,nIdent,fp);
    int r2 = genExp(e->u.bin.e2,nIdent,fp);
    int t = getNextTempVar();

    genIdent(nIdent,fp);
    fprintf(fp,"%%t%d = %ccmp %s ", t, (e->u.bin.e1->type->name == VarInt ? 'i' : 'f'), opcode);

    genType(e->u.bin.e1->type, fp);

    fprintf(fp, " %%t%d, %%t%d\nbr i1 %%t%d, label %%l%d, label %%l%d",
            r1, r2, t, lt, lf);
    return t;
}

int genCond(Exp *e, int lt, int lf, int nIdent, FILE *fp) {
    switch(e->tag){
        case ExpOr: {
            int nl = getNextTempLabel();
            genCond(e->u.bin.e1,lt,nl,nIdent,fp);
            genLabel(nl,fp);
            genCond(e->u.bin.e2,lt,lf,nIdent,fp);
            break;
        }

        case ExpNot: {
            return genCond(e->u.un,lf,lt,nIdent,fp);
        }

        //TODO: and, etc
        case ExpLess: {
            return genCondComp(e, "slt", lt, lf, nIdent, fp);
        }

        case ExpLessEqual: {
            return genCondComp(e, "sle", lt, lf, nIdent, fp);
        }

        case ExpGreater: {
            return genCondComp(e, "sgt", lt, lf, nIdent, fp);
        }

        case ExpGreaterEqual: {
            return genCondComp(e, "sge", lt, lf, nIdent, fp);
        }

        case ExpEqual: {
            int r1 = genExp(e->u.bin.e1,nIdent,fp);
            int r2 = genExp(e->u.bin.e2,nIdent,fp);
            int t = getNextTempVar();

            genIdent(nIdent,fp);
            fprintf(fp,"%%t%d = %ccmp%seq ", t, (e->u.bin.e1->type->name == VarInt ? 'i' : 'f'), (e->u.bin.e1->type->name == VarInt ? " " : " o"));

            genType(e->u.bin.e1->type, fp);

            fprintf(fp, " %%t%d, %%t%d\nbr i1 %%t%d, label %%l%d, label %%l%d",
                    r1, r2, t, lt, lf);
            return t;
        }

        default: {
            printf("Invalid type - genCond. Exiting.\n");
            exit(-1);
        }
    }

    return -3;
}

void doubleToHex(double d, FILE *strOut){
    int i;
    double f = (float) d;
    unsigned char buff[sizeof(double)];
    memcpy(buff, &f, sizeof(double));
    fprintf(strOut, "0x");
    for(i = sizeof(double) - 1; i >= 0; i--) fprintf(strOut, "%02x", buff[i]);
}

int genArith(Exp *exp, int nIdent, FILE *fp)
{
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

int genExpVar(Exp *exp, int nIdent, FILE *fp)
{
    int ret = getNextTempVar();
    char *type;

    genIdent(nIdent, fp);

    if(exp->u.var->decType == VarParam)
    {
        ret = exp->u.var->u.def.p->varNumber;
    }
    else
    {
        char *accessType;

        if(exp->u.var->u.def.dec->type->name == VarFloat)
        {
            type = "float";
        }
        else if(exp->u.var->u.def.dec->type->name == VarChar)
        {
            type = "i8";
        }
        else
        {
            type = "i32";
        }

        if(exp->u.var->u.def.dec->isGlobal)
        {
            accessType = "@";
        }
        else
        {
            accessType = "%";
        }

        fprintf(fp, "%%t%d = load %s* %st%d\n", ret, type, accessType, exp->u.var->u.def.dec->varNumber);
    }

    return ret;
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
        case ExpDiv:
            return genArith(exp, nIdent, fp);
        case ExpEqual:
        case ExpLess:
        case ExpGreater:
        case ExpLessEqual:
        case ExpGreaterEqual:
        case ExpOr:
        case ExpAnd:
        case ExpNot:
            return -4; // These are being handled on genCond function
        case ExpVar:
            return genExpVar(exp, nIdent, fp);
        case ExpCall:
            return genCmdCall(exp->u.call, nIdent, fp);
        case ExpMinus: {
            int t = genExp(exp->u.un, nIdent, fp);
            int ret = getNextTempVar();
            genIdent(nIdent,fp);
            fprintf(fp, "%%t%d = %smul ", ret, (exp->type->name == VarInt ? "" : "f"));
            genType(exp->u.un->type, fp);
            fprintf(fp, " %%t%d, -1%s\n", t, (exp->type->name == VarInt ? "" : ".0"));
            return ret;
        }
        case ExpNew:
//            printIdent(nIdent);
//            printf("Expression type: New\n");
//            printType(exp->u.newexp.type, nIdent);
//            printIdent(nIdent);
//            printf("Expression resulting ");
//            printType(exp->type, 0);
//            printExp(exp->u.newexp.exp, nIdent);
            break;
        case ExpString: {
            int temp = getNextTempVar();
            int ret = getNextTempVar();

            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = alloca i8\n", temp);
            genIdent(nIdent, fp);
            fprintf(fp, "store i8 %d, i8* %%t%d\n", exp->u.c[0], temp);
            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = load i8* %%t%d\n", ret, temp);
            return ret;
        }
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
            genIdent(nIdent, fp);
            fprintf(fp, "%%t%d = alloca float\n", temp);
            genIdent(nIdent, fp);
            fprintf(fp, "store float ");
            doubleToHex(exp->u.d, fp);
            fprintf(fp, ",float* %%t%d\n", temp);
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

int genCmdCall(CmdCall *cmd, int nIdent, FILE *fp) {
    ExpList * current;

    int ret = getNextTempVar();
    const int bufferSize = 4096;
    int buffer = bufferSize;
    char *callArgs = (char *) malloc(sizeof(char) * buffer);
    int length = 2;

    if(callArgs == NULL)
    {
        printf("Unable to allocate memory for argument list. Exiting.\n");
        exit(-1);
    }

    sprintf(callArgs,"(");

    for(current = cmd->parameters; current != NULL; current = current->next) {
        int arg = genExp(current->exp, nIdent, fp);
        char *type = getType(current->exp->type);

        length += 2 + floor(log10(abs(arg))) + 1 + 7; // 7 -> type
        if(length > buffer)
        {
            buffer += bufferSize;
            callArgs = (char *) realloc(callArgs, sizeof(char) * buffer);

            if(callArgs == NULL)
            {
                printf("Unable to reallocate memory for argument list. Exiting.\n");
                exit(-1);
            }
        }

        sprintf(callArgs + strlen(callArgs), "%s %%t%d", type, arg);
        if(current->next != NULL) sprintf(callArgs + strlen(callArgs), ",");
    }

    sprintf(callArgs + strlen(callArgs),")");

    genIdent(nIdent, fp);
    fprintf(fp,"%%t%d = call ", ret);

    genType(cmd->type, fp);

    fprintf(fp," @%s%s\n", cmd->id, callArgs);

    free(callArgs);

    return ret;
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
        case CmdBasicCall: {
            genCmdCall(cmd->u.call, nIdent, fp);
            break;
        }
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

